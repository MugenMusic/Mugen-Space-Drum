#include "AmbienceEngine.h"

void AmbienceEngine::prepare (double sampleRate)
{
    currentSampleRate = sampleRate;

    // Até 4 s de delay (uma nota inteira a 60 BPM) + margem. Toda a memória
    // é reservada aqui, nunca durante o processamento de áudio.
    const auto size = (size_t) (sampleRate * (maxDelayMs * 0.001 + 0.1)) + 4;
    delayLeft.assign (size, 0.0f);
    delayRight.assign (size, 0.0f);

    delaySamples.reset (sampleRate, 0.25);
    timeFadeStep = 1.0f / (float) (0.04 * sampleRate);
    timeFadeState = 0;
    timeFade = 1.0f;
    hasTime = false;
    delayMixSmoothed.reset (sampleRate, 0.05);

    // Os ecos perdem agudos a cada repetição (som mais quente e sonhador).
    feedbackFilterCoef = (float) (1.0 - std::exp (-juce::MathConstants<double>::twoPi * 3800.0 / sampleRate));

    reverb.prepare (sampleRate);

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

    const float target = juce::jlimit (1.0f, maxDelay, settings.delayTimeMs * 0.001f * (float) currentSampleRate);

    if (! hasTime)
    {
        delaySamples.setCurrentAndTargetValue (target);
        hasTime = true;
    }
    else if (timeFadeState != 1)
    {
        const float current = delaySamples.getTargetValue();
        if (std::abs (target - current) > 0.05f * current)
            timeFadeState = 1;                    // salto grande: faz a transição sem desafinar
        else if (timeFadeState == 0)
            delaySamples.setTargetValue (target); // pequenos ajustes no knob: desliza suavemente
    }
    delayMixSmoothed.setTargetValue (settings.delayMix);
    const float feedback = juce::jlimit (0.0f, 0.9f, settings.delayFeedback);

    for (int i = 0; i < numSamples; ++i)
    {
        if (timeFadeState == 1)
        {
            timeFade -= timeFadeStep;
            if (timeFade <= 0.0f)
            {
                timeFade = 0.0f;
                delaySamples.setCurrentAndTargetValue (target);
                timeFadeState = 2;
            }
        }
        else if (timeFadeState == 2)
        {
            timeFade += timeFadeStep * 0.5f;
            if (timeFade >= 1.0f)
            {
                timeFade = 1.0f;
                timeFadeState = 0;
            }
        }

        const float d = delaySamples.getNextValue();
        const float mix = delayMixSmoothed.getNextValue() * timeFade;

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

void AmbienceEngine::process (juce::AudioBuffer<float>& buffer, const Settings& settings)
{
    if (buffer.getNumChannels() < 2)
        return;

    const int numSamples = buffer.getNumSamples();
    float* left  = buffer.getWritePointer (0);
    float* right = buffer.getWritePointer (1);

    processDelay (left, right, numSamples, settings);

    reverb.process (left, right, numSamples, settings.reverb);
}
