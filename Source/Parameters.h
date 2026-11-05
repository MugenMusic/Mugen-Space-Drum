#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

// ==============================================================================
//  Parameter Management
//  Todos os parâmetros do Mugen Space Drum ficam definidos aqui, num só sítio.
//  Todos podem ser automatizados no Ableton e no Bitwig.
// ==============================================================================
namespace ParamIDs
{
    // Tom do instrumento
    inline constexpr const char* character     = "character";
    inline constexpr const char* brightness    = "brightness";
    inline constexpr const char* decay         = "decay";
    inline constexpr const char* resonance     = "resonance";

    // Sample Engine (samples orgânicos gerados pelo plugin)
    inline constexpr const char* sampleType    = "sampleType";
    inline constexpr const char* blend         = "blend";

    // Delay
    inline constexpr const char* delayMix      = "delayMix";
    inline constexpr const char* delayTime     = "delayTime";
    inline constexpr const char* delayFeedback = "delayFeedback";
    inline constexpr const char* delaySync     = "delaySync";
    inline constexpr const char* delayDivision = "delayDivision";

    // Reverb
    inline constexpr const char* reverbMix     = "reverbMix";
    inline constexpr const char* reverbSize    = "reverbSize";
    inline constexpr const char* reverbType    = "reverbType";
    inline constexpr const char* shimmer       = "shimmer";
    inline constexpr const char* freeze        = "freeze";

    // Motion
    inline constexpr const char* motionDepth   = "motionDepth";
    inline constexpr const char* motionRate    = "motionRate";
    inline constexpr const char* width         = "width";

    // Saída
    inline constexpr const char* volume        = "volume";
}

/** Divisões musicais do delay sincronizado e a sua duração em batidas (semínimas). */
namespace DelayDivisions
{
    inline juce::StringArray names() { return { "1/16", "1/8T", "1/8", "1/8D", "1/4T", "1/4", "1/4D", "1/2", "1/1" }; }
    inline float beats (int index)
    {
        static constexpr float values[] = { 0.25f, 1.0f / 3.0f, 0.5f, 0.75f, 2.0f / 3.0f, 1.0f, 1.5f, 2.0f, 4.0f };
        return values[juce::jlimit (0, 8, index)];
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
