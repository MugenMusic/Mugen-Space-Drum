#include "Parameters.h"

namespace
{
    using Attributes = juce::AudioParameterFloatAttributes;

    Attributes percent()
    {
        return Attributes().withStringFromValueFunction ([] (float v, int)
        {
            return juce::String (juce::roundToInt (v * 100.0f)) + " %";
        });
    }

    Attributes seconds()
    {
        return Attributes().withStringFromValueFunction ([] (float v, int)
        {
            return juce::String (v, 1) + " s";
        });
    }

    Attributes milliseconds()
    {
        return Attributes().withStringFromValueFunction ([] (float v, int)
        {
            return juce::String (juce::roundToInt (v)) + " ms";
        });
    }

    Attributes decibels()
    {
        return Attributes().withStringFromValueFunction ([] (float v, int)
        {
            return juce::String (v, 1) + " dB";
        });
    }

    std::unique_ptr<juce::AudioParameterFloat> makeParam (const char* id,
                                                          const char* name,
                                                          juce::NormalisableRange<float> range,
                                                          float defaultValue,
                                                          Attributes attributes)
    {
        return std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id, 1 },
                                                            name, range, defaultValue,
                                                            attributes);
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // --- Tom ------------------------------------------------------------------
    layout.add (makeParam (ParamIDs::brightness, "Brightness",
                           { 0.0f, 1.0f }, 0.45f, percent()));

    {
        juce::NormalisableRange<float> decayRange { 0.5f, 12.0f };
        decayRange.setSkewForCentre (3.5f);
        layout.add (makeParam (ParamIDs::decay, "Decay", decayRange, 4.5f, seconds()));
    }

    layout.add (makeParam (ParamIDs::resonance, "Resonance",
                           { 0.0f, 1.0f }, 0.55f, percent()));

    // --- Delay ----------------------------------------------------------------
    layout.add (makeParam (ParamIDs::delayMix, "Delay Mix",
                           { 0.0f, 1.0f }, 0.25f, percent()));

    {
        juce::NormalisableRange<float> timeRange { 60.0f, 1500.0f };
        timeRange.setSkewForCentre (450.0f);
        layout.add (makeParam (ParamIDs::delayTime, "Delay Time", timeRange, 420.0f, milliseconds()));
    }

    layout.add (makeParam (ParamIDs::delayFeedback, "Delay Feedback",
                           { 0.0f, 0.9f }, 0.45f, percent()));

    // --- Reverb ---------------------------------------------------------------
    layout.add (makeParam (ParamIDs::reverbMix, "Reverb Mix",
                           { 0.0f, 1.0f }, 0.40f, percent()));

    layout.add (makeParam (ParamIDs::reverbSize, "Reverb Size",
                           { 0.0f, 1.0f }, 0.80f, percent()));

    // --- Saída ----------------------------------------------------------------
    layout.add (makeParam (ParamIDs::volume, "Volume",
                           { -36.0f, 6.0f }, -6.0f, decibels()));

    return layout;
}
