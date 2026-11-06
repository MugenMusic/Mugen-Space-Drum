#include "SpaceDrumVoice.h"

namespace
{
    constexpr double silenceThreshold = 0.0001; // aprox. -80 dB

    double multiplierForTime (double seconds, double sampleRate)
    {
        // Multiplicador por amostra que faz o som descer 60 dB em "seconds".
        return std::exp (-6.907755 / (juce::jmax (0.001, seconds) * sampleRate));
    }

    double onePoleCoef (double cutoffHz, double sampleRate)
    {
        return 1.0 - std::exp (-juce::MathConstants<double>::twoPi * cutoffHz / sampleRate);
    }
}

SpaceDrumVoice::SpaceDrumVoice (VoiceParameters params)
    : parameters (params)
{
}

bool SpaceDrumVoice::canPlaySound (juce::SynthesiserSound* sound)
{
    return dynamic_cast<SpaceDrumSound*> (sound) != nullptr;
}

void SpaceDrumVoice::startNote (int midiNoteNumber, float velocity,
                                juce::SynthesiserSound*, int)
{
    const double sampleRate = getSampleRate();
    if (sampleRate <= 0.0)
        return;

    const double character      = parameters.character->load();
    const double brightnessKnob = parameters.brightness->load();
    const double decayKnob      = parameters.decay->load();
    const double resonance      = parameters.resonance->load();
    const double blend          = parameters.blend->load();
    const int    sampleType     = juce::roundToInt (parameters.sampleType->load());

    // --- Velocidade e expressão ----------------------------------------------------
    // Sensibilidade 0: todas as notas iguais. Sensibilidade 1: tocar suave dá um som
    // abafado e "fofinho" (baqueta macia, ataque redondo); tocar forte dá brilho e metal.
    const double sens        = parameters.velocitySens != nullptr ? (double) parameters.velocitySens->load() : 0.75;
    const double rawVel      = juce::jlimit (0.0, 1.0, (double) velocity);
    const double vel         = 1.0 - sens * (1.0 - rawVel);   // velocidade "sentida"
    const double timbreCurve = (1.0 - sens) + sens * (0.25 + 1.0 * rawVel * rawVel + 0.15 * rawVel);
    const double brightness  = juce::jlimit (0.0, 1.0, brightnessKnob * timbreCurve + sens * 0.25 * (rawVel - 0.6));
    const double tuneRatio   = std::pow (2.0, (parameters.fineTune != nullptr ? parameters.fineTune->load() : 0.0) / 1200.0);
    const double fundamental = juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber) * tuneRatio;
    const double twoPiOverSr = juce::MathConstants<double>::twoPi / sampleRate;

    const auto modeSet = ModalModel::forCharacter (character);

    // --- Baqueta ----------------------------------------------------------------
    // Tempo de contacto: baqueta macia toca mais tempo no metal e filtra os agudos.
    const double hardness    = 0.15 + 0.85 * brightness;
    const double contactTime = 0.0025 + (1.0 - hardness) * 0.006;
    const double fundamentalResponse = 1.0 + std::pow (fundamental * contactTime, 2.0);

    // Notas graves soam mais tempo, notas agudas um pouco menos.
    const double pitchLength = juce::jlimit (0.45, 1.8, std::pow (2.0, -(midiNoteNumber - 60) / 30.0));
    const double baseSeconds = decayKnob * pitchLength;

    for (size_t k = 0; k < (size_t) numModes; ++k)
    {
        auto& m = modes[k];

        // Pequena variação orgânica por toque (afinação ±0,07 %, volume ±6 %).
        const double humanTune  = 1.0 + (random.nextDouble() * 2.0 - 1.0) * 0.0007 * (k == 0 ? 0.0 : 1.0);
        const double humanLevel = 1.0 + (random.nextDouble() * 2.0 - 1.0) * 0.06;

        const double freq = fundamental * modeSet.ratios[k] * humanTune;

        if (freq >= sampleRate * 0.42)
        {
            m.mainAmp = m.twinAmp = 0.0;
            m.envelope = 0.0;
            continue;
        }

        const double malletFilter = std::sqrt (fundamentalResponse / (1.0 + std::pow (freq * contactTime, 2.0)));

        m.mainAmp = modeSet.levels[k] * malletFilter * humanLevel;
        m.twinAmp = m.mainAmp * resonance * 0.7;

        const double lengthBoost = (k == 0) ? 1.0 : (1.0 + resonance * 1.2);
        m.envelope = 1.0;
        m.envelopeMultiplier = multiplierForTime (baseSeconds * modeSet.lengths[k] * lengthBoost, sampleRate);

        // Gémeo ligeiramente desafinado: batimento lento e etéreo.
        const double detune = (0.0008 + 0.0035 * resonance) * ((k % 2 == 0) ? 1.0 : -1.0);
        m.mainStep = freq * twoPiOverSr;
        m.twinStep = freq * (1.0 + detune) * twoPiOverSr;
        m.main.reset();
        m.twin.reset();
    }

    // --- Descida de afinação no ataque (até ~0,5 % com toques fortes) -------------
    glideAmount     = 0.0015 + 0.0035 * vel * hardness;
    glideMultiplier = std::exp (-(double) glideUpdateInterval / (0.035 * sampleRate));
    glideCounter    = 0;
    updateGlide();

    // --- Ruído do toque: suave, nunca agressivo -------------------------------------
    malletLevel      = 0.22 * hardness * vel;
    malletMultiplier = multiplierForTime (0.012 + (1.0 - hardness) * 0.03, sampleRate);
    malletFilterCoef = onePoleCoef (600.0 + 5000.0 * hardness, sampleRate);
    malletState      = 0.0;

    // Ataque: mais redondo com baqueta macia.
    attackSamples  = juce::jmax (1, (int) (sampleRate * contactTime * 0.8));
    attackPosition = 0;

    toneFilterCoef  = onePoleCoef (juce::jmin (sampleRate * 0.45, 1500.0 + brightness * 11000.0 + fundamental * 2.0), sampleRate);
    toneFilterState = 0.0;

    // --- Sample orgânico -------------------------------------------------------------
    // Mistura de potência constante: o volume total mantém-se ao rodar o Blend.
    synthMix  = std::cos (blend * juce::MathConstants<double>::halfPi);
    sampleMix = std::sin (blend * juce::MathConstants<double>::halfPi) * 1.8;

    currentSample = (parameters.sampleBank != nullptr && sampleMix > 0.001)
                        ? parameters.sampleBank->find (sampleType, midiNoteNumber)
                        : nullptr;
    samplePosition = 0.0;
    if (currentSample != nullptr)
        sampleIncrement = std::pow (2.0, (midiNoteNumber - currentSample->rootNote) / 12.0) * tuneRatio;

    if (synthMix < 0.001)
        for (auto& m : modes)
            m.envelope = 0.0;

    outputGain = std::pow (vel, 1.4) * 0.15 / (1.0 + 0.35 * resonance);

    // Pequena abertura estéreo: graves ligeiramente à esquerda, agudos à direita.
    const float pan   = (float) juce::jlimit (-1.0, 1.0, (midiNoteNumber - 60) / 36.0) * 0.35f;
    const float angle = (pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f;
    gainLeft  = std::cos (angle);
    gainRight = std::sin (angle);

    isActive = true;
}

void SpaceDrumVoice::updateGlide() noexcept
{
    const double factor = 1.0 + glideAmount;
    for (auto& m : modes)
    {
        if (m.envelope <= 0.0)
            continue;
        m.main.setAngularStep (m.mainStep * factor);
        m.twin.setAngularStep (m.twinStep * factor);
    }
}

double SpaceDrumVoice::readSample() noexcept
{
    // Leitura com interpolação cúbica (som limpo quando o sample é afinado).
    const auto& data = currentSample->data;
    const int size = (int) data.size();
    const int i1 = (int) samplePosition;

    if (i1 >= size - 2)
    {
        currentSample = nullptr;
        return 0.0;
    }

    const double t  = samplePosition - i1;
    const double y0 = i1 > 0 ? data[(size_t) (i1 - 1)] : data[(size_t) i1];
    const double y1 = data[(size_t) i1];
    const double y2 = data[(size_t) (i1 + 1)];
    const double y3 = data[(size_t) (i1 + 2)];

    const double c1 = 0.5 * (y2 - y0);
    const double c2 = y0 - 2.5 * y1 + 2.0 * y2 - 0.5 * y3;
    const double c3 = 0.5 * (y3 - y0) + 1.5 * (y1 - y2);

    samplePosition += sampleIncrement * (1.0 + glideAmount);
    return ((c3 * t + c2) * t + c1) * t + y1;
}

void SpaceDrumVoice::stopNote (float, bool allowTailOff)
{
    // Como num Space Drum real, a nota continua a ressoar depois de largar
    // a tecla. Só paramos de imediato quando o anfitrião o exige.
    if (! allowTailOff)
    {
        isActive = false;
        clearCurrentNote();
    }
}

void SpaceDrumVoice::renderNextBlock (juce::AudioBuffer<float>& outputBuffer,
                                      int startSample, int numSamples)
{
    if (! isActive)
        return;

    const int numChannels = outputBuffer.getNumChannels();
    float* left  = outputBuffer.getWritePointer (0);
    float* right = numChannels > 1 ? outputBuffer.getWritePointer (1) : nullptr;

    for (int i = 0; i < numSamples; ++i)
    {
        // A afinação desce suavemente até ao valor certo nos primeiros ~100 ms.
        if (glideAmount > 1.0e-6 && ++glideCounter >= glideUpdateInterval)
        {
            glideCounter = 0;
            glideAmount *= glideMultiplier;
            updateGlide();
        }

        double sample = 0.0;

        for (auto& m : modes)
        {
            if (m.envelope <= 0.0)
                continue;

            sample += (m.main.next() * m.mainAmp + m.twin.next() * m.twinAmp) * m.envelope;
            m.envelope *= m.envelopeMultiplier;
        }

        if (malletLevel > 1.0e-6)
        {
            const double noise = random.nextDouble() * 2.0 - 1.0;
            malletState += (noise - malletState) * malletFilterCoef;
            sample += malletState * malletLevel;
            malletLevel *= malletMultiplier;
        }

        if (attackPosition < attackSamples)
        {
            sample *= (double) attackPosition / (double) attackSamples;
            ++attackPosition;
        }

        sample *= synthMix;

        if (currentSample != nullptr)
            sample += readSample() * sampleMix;

        toneFilterState += (sample - toneFilterState) * toneFilterCoef;
        const float out = (float) (toneFilterState * outputGain);

        left[startSample + i] += out * gainLeft;
        if (right != nullptr)
            right[startSample + i] += out * gainRight;
    }

    // Manutenção dos osciladores e verificação de silêncio.
    bool stillSounding = malletLevel > silenceThreshold || currentSample != nullptr;

    for (auto& m : modes)
    {
        m.main.normalise();
        m.twin.normalise();

        if (m.envelope > silenceThreshold)
            stillSounding = true;
        else
            m.envelope = 0.0;
    }

    if (! stillSounding)
    {
        isActive = false;
        clearCurrentNote();
    }
}
