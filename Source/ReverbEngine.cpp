#include "ReverbEngine.h"

namespace
{
    struct TypePreset
    {
        float lengthScale;   // tamanho do "espaço"
        float rtMin, rtMax;  // tempo de reverberação (s) com Size a 0 e a 1
        float preDelayMs;
        float dampHz;        // abaixo disto os ecos mantêm o brilho
        float diffusion;
        float modMs;         // profundidade da modulação da cauda
        float modRate;       // Hz
    };

    const std::array<TypePreset, ReverbEngine::numTypes> presets {{
        { 0.45f, 0.4f,  1.4f,  4.0f,  6000.0f, 0.55f, 0.15f, 0.60f },  // Room
        { 1.00f, 1.5f,  4.0f,  18.0f, 7000.0f, 0.65f, 0.35f, 0.35f },  // Hall
        { 1.50f, 3.0f,  8.0f,  32.0f, 5000.0f, 0.70f, 0.50f, 0.25f },  // Cathedral
        { 2.10f, 5.0f, 14.0f,  55.0f, 7500.0f, 0.75f, 0.90f, 0.15f }   // Deep Space
    }};

    // Comprimentos base das 8 linhas (ms), escolhidos para não terem múltiplos comuns.
    constexpr std::array<float, 8> baseLengthsMs { 29.7f, 37.1f, 41.1f, 43.7f, 53.3f, 59.9f, 67.7f, 73.1f };

    constexpr std::array<float, 4> diffuserMsLeft  { 4.77f, 3.59f, 12.73f, 9.31f };
    constexpr std::array<float, 4> diffuserMsRight { 4.93f, 3.71f, 12.13f, 9.73f };

    // Combinações de saída diferentes para a esquerda e a direita (som largo).
    constexpr std::array<float, 8> outSignsLeft  {  1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f };
    constexpr std::array<float, 8> outSignsRight {  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f };
}

// ------------------------------------------------------------------------------
float ReverbEngine::OctaveShifter::process (float x) noexcept
{
    line.push (x);

    float phase2 = phase + 0.5f;
    if (phase2 >= 1.0f)
        phase2 -= 1.0f;

    const float tap1 = line.read (grainSamples * (1.0f - phase)  + 2.0f);
    const float tap2 = line.read (grainSamples * (1.0f - phase2) + 2.0f);

    const float out = tap1 * std::sin (juce::MathConstants<float>::pi * phase)
                    + tap2 * std::sin (juce::MathConstants<float>::pi * phase2);

    // Ler o dobro da velocidade = uma oitava acima.
    phase += 1.0f / grainSamples;
    if (phase >= 1.0f)
        phase -= 1.0f;

    return out;
}

// ------------------------------------------------------------------------------
void ReverbEngine::prepare (double sampleRate)
{
    currentSampleRate = sampleRate;

    for (auto& line : lines)
        line.allocate ((int) (0.3 * sampleRate));

    for (size_t i = 0; i < (size_t) numDiffusers; ++i)
    {
        diffusersLeft[i].line.allocate ((int) (0.03 * sampleRate));
        diffusersRight[i].line.allocate ((int) (0.03 * sampleRate));
        diffusersLeft[i].delay  = diffuserMsLeft[i]  * 0.001f * (float) sampleRate;
        diffusersRight[i].delay = diffuserMsRight[i] * 0.001f * (float) sampleRate;
    }

    preDelayLeft.allocate ((int) (0.1 * sampleRate));
    preDelayRight.allocate ((int) (0.1 * sampleRate));

    shimmerShifter.grainSamples = (float) (0.08 * sampleRate);
    shimmerShifter.line.allocate ((int) shimmerShifter.grainSamples + 8);

    for (auto& l : lineLengths)
        l.reset (sampleRate, 0.5);

    preDelaySamples.reset (sampleRate, 0.3);
    wetGain.reset (sampleRate, 0.05);
    dryGain.reset (sampleRate, 0.05);
    inputGain.reset (sampleRate, 0.05);
    shimmerAmount.reset (sampleRate, 0.1);
    freezeAmount.reset (sampleRate, 0.2);

    lastType = -1;
    lastSize = -1.0f;
    changeFadeState = 0;
    changeFade = 1.0f;
    changeFadeStep = 1.0f / (float) (0.06 * sampleRate);

    reset();
}

void ReverbEngine::snap (const Settings& settings)
{
    reset();
    lastType = -1;     // força o reposicionamento imediato (sem deslizar)
    lastSize = -1.0f;
    updateTargets (settings);
}

void ReverbEngine::reset()
{
    for (auto& line : lines)
        line.clear();
    for (auto& d : diffusersLeft)
        d.line.clear();
    for (auto& d : diffusersRight)
        d.line.clear();

    preDelayLeft.clear();
    preDelayRight.clear();
    shimmerShifter.line.clear();
    shimmerShifter.phase = 0.0f;

    dampState.fill (0.0f);
    for (size_t i = 0; i < lfoPhase.size(); ++i)
        lfoPhase[i] = (float) i * 0.785f;

    shimmerFilterLow = shimmerFilterHigh = shimmerFeedback = 0.0f;
}

void ReverbEngine::updateTargets (const Settings& settings)
{
    const int type = juce::jlimit (0, numTypes - 1, settings.type);
    const auto& p = presets[(size_t) type];
    const float sr = (float) currentSampleRate;
    const bool firstTime = lastType < 0;

    if (type != lastType || std::abs (settings.size - lastSize) > 0.0005f)
    {
        const float scale = p.lengthScale * (0.75f + 0.5f * settings.size);
        const float rt60  = p.rtMin + (p.rtMax - p.rtMin) * settings.size;

        for (size_t i = 0; i < (size_t) numLines; ++i)
        {
            const float lengthSamples = baseLengthsMs[i] * 0.001f * sr * scale;
            if (firstTime)
                lineLengths[i].setCurrentAndTargetValue (lengthSamples);
            else
                lineLengths[i].setTargetValue (lengthSamples);

            // Ganho que faz a energia cair 60 dB em rt60 segundos.
            lineGain[i] = std::pow (10.0f, -3.0f * lengthSamples / (rt60 * sr));
        }

        shimmerLoopGain = 2.0f * juce::jmin (1.0f, 3.0f / rt60);

        const float preDelay = juce::jmax (1.0f, p.preDelayMs * 0.001f * sr);
        if (firstTime)
            preDelaySamples.setCurrentAndTargetValue (preDelay);
        else
            preDelaySamples.setTargetValue (preDelay);

        dampCoef = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * p.dampHz / sr);
        modDepthSamples = p.modMs * 0.001f * sr;
        lfoIncrement = juce::MathConstants<float>::twoPi * p.modRate / sr;

        for (auto& d : diffusersLeft)
            d.gain = p.diffusion;
        for (auto& d : diffusersRight)
            d.gain = p.diffusion;

        lastType = type;
        lastSize = settings.size;
    }

    const float freeze = settings.freeze ? 1.0f : 0.0f;
    freezeAmount.setTargetValue (freeze);
    inputGain.setTargetValue (1.0f - freeze);
    shimmerAmount.setTargetValue (settings.shimmer * (1.0f - freeze));
    wetGain.setTargetValue (settings.mix * 1.4f);
    dryGain.setTargetValue (1.0f - settings.mix * 0.4f);
}

void ReverbEngine::process (float* left, float* right, int numSamples, const Settings& settings)
{
    if (lines[0].buffer.empty())
        return;

    // Mudança grande? Desvanece primeiro, muda em silêncio, depois volta.
    const bool bigChange = lastType >= 0
                        && (settings.type != lastType || std::abs (settings.size - lastSize) > 0.25f);
    if (bigChange && changeFadeState != 1)
        changeFadeState = 1;   // (também se chegar outra mudança a meio do regresso)

    if (changeFadeState == 1)
    {
        Settings held = settings;   // durante o fade mantém a sala antiga
        held.type = lastType;
        held.size = lastSize;
        updateTargets (held);
    }
    else
    {
        updateTargets (settings);
    }

    const float shimmerHighpass = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 400.0f  / (float) currentSampleRate);
    const float shimmerLowpass  = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 6000.0f / (float) currentSampleRate);

    for (int n = 0; n < numSamples; ++n)
    {
        const float freeze  = freezeAmount.getNextValue();
        const float inGain  = inputGain.getNextValue();
        const float shimmer = shimmerAmount.getNextValue();
        if (changeFadeState == 1)
            changeFade = juce::jmax (0.0f, changeFade - changeFadeStep);
        else if (changeFadeState == 2)
        {
            changeFade = juce::jmin (1.0f, changeFade + changeFadeStep * 0.5f);
            if (changeFade >= 1.0f)
                changeFadeState = 0;
        }
        const float wet     = wetGain.getNextValue() * changeFade;
        const float dry     = dryGain.getNextValue();
        const float preDelay = preDelaySamples.getNextValue();

        // --- Entrada: pré-atraso + shimmer + difusão ------------------------------
        preDelayLeft.push (left[n] * inGain);
        preDelayRight.push (right[n] * inGain);

        float a = preDelayLeft.read (preDelay)  + shimmerFeedback;
        float b = preDelayRight.read (preDelay) + shimmerFeedback;

        for (auto& d : diffusersLeft)
            a = d.process (a);
        for (auto& d : diffusersRight)
            b = d.process (b);

        // --- Rede de 8 linhas ----------------------------------------------------
        const float damp = dampCoef + (1.0f - dampCoef) * freeze;   // Freeze: sem perda de agudos
        std::array<float, numLines> out {};
        float sum = 0.0f;

        for (size_t i = 0; i < (size_t) numLines; ++i)
        {
            lfoPhase[i] += lfoIncrement * (1.0f + 0.13f * (float) i);
            if (lfoPhase[i] > juce::MathConstants<float>::twoPi)
                lfoPhase[i] -= juce::MathConstants<float>::twoPi;

            const float length = lineLengths[i].getNextValue() + modDepthSamples * (1.0f + std::sin (lfoPhase[i]));
            const float y = lines[i].read (length);

            dampState[i] += (y - dampState[i]) * damp;
            out[i] = dampState[i];
            sum += out[i];
        }

        // Matriz de Householder: mistura a energia entre todas as linhas sem a perder.
        const float mixTerm = sum * (2.0f / (float) numLines);
        float wetLeft = 0.0f, wetRight = 0.0f;

        for (size_t i = 0; i < (size_t) numLines; ++i)
        {
            const float gain = lineGain[i] + (1.0f - lineGain[i]) * freeze;  // Freeze: a cauda não morre
            const float input = ((i % 2 == 0) ? a : b) * ((i % 4 < 2) ? 0.35f : -0.35f);
            lines[i].push ((out[i] - mixTerm) * gain + input);

            wetLeft  += out[i] * outSignsLeft[i];
            wetRight += out[i] * outSignsRight[i];
        }

        wetLeft  *= 0.7f;
        wetRight *= 0.7f;

        // --- Shimmer: a cauda afinada uma oitava acima volta a entrar ---------------
        if (shimmer > 0.0001f)
        {
            const float mono = 0.5f * (wetLeft + wetRight);
            shimmerFilterHigh += (mono - shimmerFilterHigh) * shimmerHighpass;
            const float shifted = shimmerShifter.process (mono - shimmerFilterHigh);
            shimmerFilterLow += (shifted - shimmerFilterLow) * shimmerLowpass;
            shimmerFeedback = std::tanh (shimmerFilterLow * shimmer * shimmerLoopGain);
        }
        else
        {
            shimmerFeedback = 0.0f;
        }

        left[n]  = left[n]  * dry + wetLeft  * wet;
        right[n] = right[n] * dry + wetRight * wet;
    }

    if (changeFadeState == 1 && changeFade <= 0.0f)
    {
        snap (settings);
        changeFadeState = 2;
    }
}

