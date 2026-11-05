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

    // Reverb
    inline constexpr const char* reverbMix     = "reverbMix";
    inline constexpr const char* reverbSize    = "reverbSize";

    // Saída
    inline constexpr const char* volume        = "volume";
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
