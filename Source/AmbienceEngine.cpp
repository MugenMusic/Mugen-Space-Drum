#include "AmbienceEngine.h"

void AmbienceEngine::prepare (double sampleRate)
{
    currentSampleRate = sampleRate;

    // Até 1,5 s de delay + margem. Toda a memória é reservada aqui,
    // nunca durante o processamento de áudio.
    const auto size = (size_t) (sampleRate * 2.0) + 4;
    delayLeft.assign (size, 0.0f);
    delayRight.assign (size, 0.0f);

    delaySamples.reset (sampleRate, 0.25);
    delayMixSmoothed.reset (sampleRate, 0.05);

    // Os ecos perdem agudos a cada repetição (som mais quente e sonhador).
    feedbackFilterCoef = (float) (1.0 - std::exp (-juce::MathConstants<double>::twoPi * 3800.0 / sampleRate));

    reverb.setSampleRate (sampleRate);
    reverbNeedsUpdate = true;

    reset();
}

void AmbienceEngine::reset()
{
    std::fill (delayLeft.begin(), delayLeft.end(), 0.0f);
    std::fill (delayRight.begin(), delayRight.end(), 0.0f);
    writeIndex = 0;
    feedbackFilterLeft = feedbackFilterRight = 0.0f;
    reverb.reset();
}

float AmbienceEngine::readDelay (const std::vector<float>& line, float delayInSamples) const
{
    const int size = (int) line.size();
    float readPosition = (float) writeIndex - delayInSamples;
    while (readPosition < 0.0f)
        readPosition += (float) size;

    const int indexA = (int) readPosition;
    const int indexB = (indexA + 1) % size;
    const float fraction = readPosition - (float) indexA;

    return line[(size_t) indexA] + fraction * (line[(size_t) indexB] - line[(size_t) indexA]);
}

void AmbienceEngine::processDelay (float* left, float* right, int numSamples, const Settings& settings)
{
    if (delayLeft.empty())
        return;

    const int size = (int) delayLeft.size();
    const float maxDelay = (float) (size - 4);

    delaySamples.setTargetValue (juce::jlimit (1.0f, maxDelay,
                                               settings.delayTimeMs * 0.001f * (float) currentSampleRate));
    delayMixSmoothed.setTargetValue (settings.delayMix);
    const float feedback = juce::jlimit (0.0f, 0.9f, settings.delayFeedback);

    for (int i = 0; i < numSamples; ++i)
    {
        const float d = delaySamples.getNextValue();
        const float mix = delayMixSmoothed.getNextValue();

        const float echoLeft  = readDelay (delayLeft, d);
        const float echoRight = readDelay (delayRight, d);

        feedbackFilterLeft  += (echoLeft  - feedbackFilterLeft)  * feedbackFilterCoef;
        feedbackFilterRight += (echoRight - feedbackFilterRight) * feedbackFilterCoef;

        const float input = 0.5f * (left[i] + right[i]);

        // Ping-pong: a entrada vai para a esquerda, cada eco salta para o outro lado.
        delayLeft[(size_t) writeIndex]  = input + feedbackFilterRight * feedback;
        delayRight[(size_t) writeIndex] = feedbackFilterLeft * feedback;

        left[i]  += feedbackFilterLeft  * mix;
        right[i] += feedbackFilterRight * mix;

        if (++writeIndex >= size)
            writeIndex = 0;
    }
}

void AmbienceEngine::updateReverb (const Settings& settings)
{
    juce::Reverb::Parameters p;
    p.roomSize   = 0.55f + settings.reverbSize * 0.43f;
    p.damping    = 0.30f + (1.0f - settings.reverbSize) * 0.30f;
    p.wetLevel   = settings.reverbMix * 0.45f;
    p.dryLevel   = 1.0f - settings.reverbMix * 0.40f;
    p.width      = 1.0f;
    p.freezeMode = 0.0f;

    const bool changed = reverbNeedsUpdate
                      || p.roomSize != lastReverbParameters.roomSize
                      || p.damping  != lastReverbParameters.damping
                      || p.wetLevel != lastReverbParameters.wetLevel
                      || p.dryLevel != lastReverbParameters.dryLevel;

    if (changed)
    {
        reverb.setParameters (p);
        lastReverbParameters = p;
        reverbNeedsUpdate = false;
    }
}

void AmbienceEngine::process (juce::AudioBuffer<float>& buffer, const Settings& settings)
{
    if (buffer.getNumChannels() < 2)
        return;

    const int numSamples = buffer.getNumSamples();
    float* left  = buffer.getWritePointer (0);
    float* right = buffer.getWritePointer (1);

    processDelay (left, right, numSamples, settings);

    updateReverb (settings);
    reverb.processStereo (left, right, numSamples);
}
