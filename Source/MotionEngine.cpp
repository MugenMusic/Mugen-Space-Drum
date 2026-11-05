#include "MotionEngine.h"

namespace
{
    constexpr float baseDelayMs  = 14.0f;  // atraso central do coro
    constexpr float sweepMs      = 3.0f;   // quanto o atraso oscila (afinação muito ligeira)
    constexpr float maxPanAmount = 0.35f;  // movimento esquerda/direita máximo
}

void MotionEngine::prepare (double sampleRate)
{
    currentSampleRate = sampleRate;
    const auto size = (size_t) (sampleRate * 0.05) + 4;
    lineLeft.assign (size, 0.0f);
    lineRight.assign (size, 0.0f);

    depthSmoothed.reset (sampleRate, 0.1);
    widthSmoothed.reset (sampleRate, 0.1);
    reset();
}

void MotionEngine::reset()
{
    std::fill (lineLeft.begin(), lineLeft.end(), 0.0f);
    std::fill (lineRight.begin(), lineRight.end(), 0.0f);
    writeIndex = 0;
    lfoPhase = 0.0;
}

float MotionEngine::read (const std::vector<float>& line, float delaySamples) const noexcept
{
    const int size = (int) line.size();
    float pos = (float) writeIndex - delaySamples;
    while (pos < 0.0f)
        pos += (float) size;
    const int i0 = (int) pos;
    const int i1 = (i0 + 1) % size;
    const float frac = pos - (float) i0;
    return line[(size_t) i0] + frac * (line[(size_t) i1] - line[(size_t) i0]);
}

void MotionEngine::processMotion (juce::AudioBuffer<float>& buffer, const Settings& settings)
{
    if (lineLeft.empty() || buffer.getNumChannels() < 2)
        return;

    float* left  = buffer.getWritePointer (0);
    float* right = buffer.getWritePointer (1);
    const int numSamples = buffer.getNumSamples();
    const int size = (int) lineLeft.size();

    depthSmoothed.setTargetValue (juce::jlimit (0.0f, 1.0f, settings.depth));
    const double increment = juce::MathConstants<double>::twoPi
                           * juce::jlimit (0.02, 2.0, (double) settings.rateHz) / currentSampleRate;

    const float msToSamples = 0.001f * (float) currentSampleRate;

    for (int n = 0; n < numSamples; ++n)
    {
        const float depth = depthSmoothed.getNextValue();

        lineLeft[(size_t) writeIndex]  = left[n];
        lineRight[(size_t) writeIndex] = right[n];

        lfoPhase += increment;
        if (lfoPhase > juce::MathConstants<double>::twoPi)
            lfoPhase -= juce::MathConstants<double>::twoPi;

        // Esquerda e direita oscilam em quadratura (90°): movimento circular.
        const float s = (float) std::sin (lfoPhase);
        const float c = (float) std::cos (lfoPhase);

        if (++writeIndex >= size)
            writeIndex = 0;

        if (depth < 0.0001f)
            continue;

        const float chorusLeft  = read (lineLeft,  (baseDelayMs + sweepMs * s) * msToSamples);
        const float chorusRight = read (lineRight, (baseDelayMs + sweepMs * c) * msToSamples);

        const float wet = depth * 0.5f;
        float l = left[n]  * (1.0f - wet * 0.5f) + chorusLeft  * wet;
        float r = right[n] * (1.0f - wet * 0.5f) + chorusRight * wet;

        // Movimento lento entre esquerda e direita.
        const float pan = s * depth * maxPanAmount;
        left[n]  = l * (1.0f - pan);
        right[n] = r * (1.0f + pan);
    }
}

void MotionEngine::processWidth (juce::AudioBuffer<float>& buffer, const Settings& settings)
{
    if (buffer.getNumChannels() < 2)
        return;

    float* left  = buffer.getWritePointer (0);
    float* right = buffer.getWritePointer (1);
    widthSmoothed.setTargetValue (juce::jlimit (0.0f, 2.0f, settings.width));

    for (int n = 0; n < buffer.getNumSamples(); ++n)
    {
        const float width = widthSmoothed.getNextValue();
        const float mid  = 0.5f * (left[n] + right[n]);
        const float side = 0.5f * (left[n] - right[n]) * width;
        left[n]  = mid + side;
        right[n] = mid - side;
    }
}
