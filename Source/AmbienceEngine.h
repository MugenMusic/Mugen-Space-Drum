#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include "ReverbEngine.h"

// ==============================================================================
//  Ambience Engine
//  Delay estéreo "ping-pong" quente (os ecos saltam entre esquerda e direita
//  e vão ficando mais suaves) seguido do Reverb Engine do Mugen
//  (tipos de sala, Shimmer e Freeze).
// ==============================================================================
class AmbienceEngine
{
public:
    struct Settings
    {
        float delayMix      = 0.25f;
        float delayTimeMs   = 420.0f;   // já convertido de divisão musical quando o Sync está ligado
        float delayFeedback = 0.45f;
        ReverbEngine::Settings reverb;
    };

    static constexpr float maxDelayMs = 4000.0f;

    void prepare (double sampleRate);
    void reset();
    void process (juce::AudioBuffer<float>& buffer, const Settings& settings);

private:
    void processDelay (float* left, float* right, int numSamples, const Settings& settings);
    float readDelay (const std::vector<float>& line, float delayInSamples) const;

    double currentSampleRate = 44100.0;

    std::vector<float> delayLeft, delayRight;
    int writeIndex = 0;
    juce::SmoothedValue<float> delaySamples;
    juce::SmoothedValue<float> delayMixSmoothed;
    float feedbackFilterLeft = 0.0f, feedbackFilterRight = 0.0f, feedbackFilterCoef = 0.3f;

    ReverbEngine reverb;
};
