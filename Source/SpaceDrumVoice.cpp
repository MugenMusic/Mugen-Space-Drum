#include "SpaceDrumVoice.h"

namespace
{
    // Proporções de frequência dos parciais. Os três primeiros seguem a
    // afinação típica de uma língua de tambor (fundamental, oitava, quinta
    // acima da oitava); os restantes são inarmónicos, dando o brilho metálico.
    constexpr std::array<double, SpaceDrumVoice::numPartials> partialRatios   { 1.0, 2.0, 3.01, 4.23, 5.41, 6.93 };

    // Volume relativo de cada parcial.
    constexpr std::array<double, SpaceDrumVoice::numPartials> partialLevels   { 1.0, 0.42, 0.22, 0.11, 0.06, 0.035 };

    // Duração relativa de cada parcial (1.0 = valor do knob Decay).
    constexpr std::array<double, SpaceDrumVoice::numPartials> partialLengths  { 1.0, 0.62, 0.40, 0.22, 0.14, 0.09 };

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

    const double brightnessKnob = parameters.brightness->load();
    const double decayKnob      = parameters.decay->load();
    const double resonance      = parameters.resonance->load();

    const double vel        = juce::jlimit (0.0, 1.0, (double) velocity);
    const double brightness = brightnessKnob * (0.55 + 0.45 * vel); // tocar mais forte = mais brilho
    const double fundamental = juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber);

    // Notas graves soam mais tempo, notas agudas um pouco menos.
    const double pitchLength = juce::jlimit (0.45, 1.8, std::pow (2.0, -(midiNoteNumber - 60) / 30.0));
    const double baseSeconds = decayKnob * pitchLength;

    for (int k = 0; k < numPartials; ++k)
    {
        auto& p = partials[(size_t) k];
        const double freq = fundamental * partialRatios[(size_t) k];

        if (freq >= sampleRate * 0.45)
        {
            p.mainAmp = p.shimmerAmp = 0.0;
            p.envelope = 0.0;
            continue;
        }

        const double tilt = (k == 0) ? 1.0 : std::pow (0.12 + 0.88 * brightness, k * 0.7);
        p.mainAmp    = partialLevels[(size_t) k] * tilt;
        p.shimmerAmp = p.mainAmp * resonance * 0.7;

        const double lengthBoost = (k == 0) ? 1.0 : (1.0 + resonance * 1.2);
        p.envelope = 1.0;
        p.envelopeMultiplier = multiplierForTime (baseSeconds * partialLengths[(size_t) k] * lengthBoost, sampleRate);

        // Desafinação muito pequena: cria o batimento lento e etéreo.
        const double detune = (0.0008 + 0.0035 * resonance) * ((k % 2 == 0) ? 1.0 : -1.0);
        p.main.setFrequency (freq, sampleRate);
        p.shimmer.setFrequency (freq * (1.0 + detune), sampleRate);
    }

    // Toque da baqueta: suave, nunca agressivo.
    malletLevel      = 0.25 * brightness * vel;
    malletMultiplier = multiplierForTime (0.03, sampleRate);
    malletFilterCoef = onePoleCoef (800.0 + 5000.0 * brightness, sampleRate);
    malletState      = 0.0;

    attackSamples  = juce::jmax (1, (int) (sampleRate * (0.002 + (1.0 - brightness) * 0.008)));
    attackPosition = 0;

    toneFilterCoef  = onePoleCoef (juce::jmin (sampleRate * 0.45, 1200.0 + brightness * 10000.0 + fundamental * 2.0), sampleRate);
    toneFilterState = 0.0;

    outputGain = std::pow (vel, 1.4) * 0.16 / (1.0 + 0.35 * resonance);

    // Pequena abertura estéreo: graves ligeiramente à esquerda, agudos à direita.
    const float pan   = (float) juce::jlimit (-1.0, 1.0, (midiNoteNumber - 60) / 36.0) * 0.35f;
    const float angle = (pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f;
    gainLeft  = std::cos (angle);
    gainRight = std::sin (angle);

    isActive = true;
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
        double sample = 0.0;

        for (auto& p : partials)
        {
            if (p.envelope <= 0.0)
                continue;

            sample += (p.main.next() * p.mainAmp + p.shimmer.next() * p.shimmerAmp) * p.envelope;
            p.envelope *= p.envelopeMultiplier;
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

        toneFilterState += (sample - toneFilterState) * toneFilterCoef;
        const float out = (float) (toneFilterState * outputGain);

        left[startSample + i] += out * gainLeft;
        if (right != nullptr)
            right[startSample + i] += out * gainRight;
    }

    // Manutenção dos osciladores e verificação de silêncio.
    bool stillSounding = malletLevel > silenceThreshold;

    for (auto& p : partials)
    {
        p.main.normalise();
        p.shimmer.normalise();

        if (p.envelope > silenceThreshold)
            stillSounding = true;
        else
            p.envelope = 0.0;
    }

    if (! stillSounding)
    {
        isActive = false;
        clearCurrentNote();
    }
}
