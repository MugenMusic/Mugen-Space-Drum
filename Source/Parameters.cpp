#include "Parameters.h"
#include "SampleBank.h"
#include "ReverbEngine.h"
#include "ScaleEngine.h"
#include "GhostEngine.h"

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

    // --- Scale & Tuning -------------------------------------------------------
    layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { ParamIDs::scaleRoot, 1 },
                                                              "Root", ScaleEngine::rootNames(),
                                                              ScaleEngine::defaultRoot));

    layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { ParamIDs::scaleType, 1 },
                                                              "Scale", ScaleEngine::scaleNames(),
                                                              ScaleEngine::defaultScale));

    layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { ParamIDs::scaleMapping, 1 },
                                                              "Mapping", ScaleEngine::mappingNames(),
                                                              (int) ScaleEngine::nearest));

    layout.add (std::make_unique<juce::AudioParameterInt> (juce::ParameterID { ParamIDs::octave, 1 },
                                                           "Octave", -2, 2, 0,
                                                           juce::AudioParameterIntAttributes().withStringFromValueFunction ([] (int v, int)
                                                           {
                                                               return v > 0 ? "+" + juce::String (v) : juce::String (v);
                                                           })));

    layout.add (makeParam (ParamIDs::fineTune, "Fine Tune",
                           { -50.0f, 50.0f }, 0.0f,
                           Attributes().withStringFromValueFunction ([] (float v, int)
                           {
                               const int cents = juce::roundToInt (v);
                               return (cents > 0 ? "+" : "") + juce::String (cents) + " ct";
                           })));

    // --- Expressão -------------------------------------------------------------
    layout.add (makeParam (ParamIDs::velocitySens, "Velocity",
                           { 0.0f, 1.0f }, 0.75f, percent()));
    layout.add (makeParam (ParamIDs::wheelAmount, "Mod Wheel",
                           { 0.0f, 1.0f }, 0.7f, percent()));
    layout.add (makeParam (ParamIDs::pressureAmount, "Aftertouch",
                           { 0.0f, 1.0f }, 0.7f, percent()));

    // --- Ghost Notes ----------------------------------------------------------
    layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { ParamIDs::ghostOn, 1 },
                                                            "Ghost Notes", false));
    layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { ParamIDs::ghostRate, 1 },
                                                              "Ghost Rate", GhostEngine::rateNames(), 2));
    layout.add (makeParam (ParamIDs::ghostDensity, "Ghost Density",
                           { 0.0f, 1.0f }, 0.55f, percent()));
    layout.add (std::make_unique<juce::AudioParameterInt> (juce::ParameterID { ParamIDs::ghostRange, 1 },
                                                           "Ghost Range", 1, 3, 2,
                                                           juce::AudioParameterIntAttributes().withStringFromValueFunction ([] (int v, int)
                                                           {
                                                               return juce::String (v) + " oct";
                                                           })));

    // --- Saída ----------------------------------------------------------------
    layout.add (makeParam (ParamIDs::volume, "Volume",
                           { -36.0f, 6.0f }, -6.0f, decibels()));

    return layout;
}
