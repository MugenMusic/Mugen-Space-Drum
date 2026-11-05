#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>

// ==============================================================================
//  Motion Engine (Fase 4)
//
//  Movimento lento e hipnótico do som:
//    - Depth : quanto o som "flutua" — um coro muito suave (duas cópias com
//              afinação a oscilar ligeiramente) e um movimento lento entre
//              esquerda e direita.
//    - Rate  : velocidade desse movimento (de muito lento a ondulante).
//    - Width : largura estéreo final (0 % = mono, 100 % = normal, 200 % = muito largo).
// ==============================================================================
class MotionEngine
{
public:
    struct Settings
    {
        float depth = 0.3f;    // 0..1
        float rateHz = 0.2f;   // 0.02..2 Hz
        float width = 1.0f;    // 0..2
    };

    void prepare (double sampleRate);
    void reset();

    /** Coro suave + movimento estéreo (antes do delay e do reverb). */
    void processMotion (juce::AudioBuffer<float>& buffer, const Settings& settings);

    /** Largura estéreo (no fim de tudo). */
    void processWidth (juce::AudioBuffer<float>& buffer, const Settings& settings);

private:
    float read (const std::vector<float>& line, float delaySamples) const noexcept;

    double currentSampleRate = 44100.0;
    std::vector<float> lineLeft, lineRight;
    int writeIndex = 0;
    double lfoPhase = 0.0;

    juce::SmoothedValue<float> depthSmoothed, widthSmoothed;
};
