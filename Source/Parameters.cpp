#include "Parameters.h"
#include "SampleBank.h"
#include "ReverbEngine.h"

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
    layout.add (makeParam (ParamIDs::character, "Character",
                           { 0.0f, 1.0f }, 0.5f,
                           Attributes().withStringFromValueFunction ([] (float v, int)
                           {
                               const juce::String body = v < 0.25f ? "Tongue" : (v < 0.75f ? "Handpan" : "Crystal");
                               return body + " " + juce::String (juce::roundToInt (v * 100.0f)) + "%";
                           })));

    layout.add (makeParam (ParamIDs::brightness, "Brightness",
                           { 0.0f, 1.0f }, 0.45f, percent()));

    {
        juce::NormalisableRange<float> decayRange { 0.5f, 12.0f };
        decayRange.setSkewForCentre (3.5f);
        layout.add (makeParam (ParamIDs::decay, "Decay", decayRange, 4.5f, seconds()));
    }

    layout.add (makeParam (ParamIDs::resonance, "Resonance",
                           { 0.0f, 1.0f }, 0.55f, percent()));

    // --- Sample Engine --------------------------------------------------------
    layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { ParamIDs::sampleType, 1 },
                                                              "Sample", SampleBank::typeNames(),
                                                              (int) SampleBank::shell));

    layout.add (makeParam (ParamIDs::blend, "Blend",
                           { 0.0f, 1.0f }, 0.35f,
                           Attributes().withStringFromValueFunction ([] (float v, int)
                           {
                               const int samplePercent = juce::roundToInt (v * 100.0f);
                               return juce::String (100 - samplePercent) + " / " + juce::String (samplePercent);
                           })));

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

    layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { ParamIDs::delaySync, 1 },
                                                            "Delay Sync", false));

    layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { ParamIDs::delayDivision, 1 },
                                                              "Delay Division", DelayDivisions::names(), 3));

    // --- Reverb ---------------------------------------------------------------
    layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { ParamIDs::reverbType, 1 },
                                                              "Reverb Type", ReverbEngine::typeNames(),
                                                              (int) ReverbEngine::hall));

    layout.add (makeParam (ParamIDs::shimmer, "Shimmer",
                           { 0.0f, 1.0f }, 0.2f, percent()));

    layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { ParamIDs::freeze, 1 },
                                                            "Freeze", false));

    layout.add (makeParam (ParamIDs::reverbMix, "Reverb Mix",
                           { 0.0f, 1.0f }, 0.40f, percent()));

    layout.add (makeParam (ParamIDs::reverbSize, "Reverb Size",
                           { 0.0f, 1.0f }, 0.80f, percent()));

    // --- Motion ---------------------------------------------------------------
    layout.add (makeParam (ParamIDs::motionDepth, "Motion Depth",
                           { 0.0f, 1.0f }, 0.3f, percent()));

    {
        juce::NormalisableRange<float> rateRange { 0.02f, 2.0f };
        rateRange.setSkewForCentre (0.25f);
        layout.add (makeParam (ParamIDs::motionRate, "Motion Rate", rateRange, 0.15f,
                               Attributes().withStringFromValueFunction ([] (float v, int)
                               {
                                   return juce::String (v, 2) + " Hz";
                               })));
    }

    layout.add (makeParam (ParamIDs::width, "Width",
                           { 0.0f, 2.0f }, 1.2f, percent()));

    // --- Saída ----------------------------------------------------------------
    layout.add (makeParam (ParamIDs::volume, "Volume",
                           { -36.0f, 6.0f }, -6.0f, decibels()));

    return layout;
}
