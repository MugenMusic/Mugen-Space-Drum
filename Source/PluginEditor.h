#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "MugenLookAndFeel.h"

// ==============================================================================
//  PluginEditor — a janela do plugin.
// ==============================================================================
class MugenSpaceDrumEditor : public juce::AudioProcessorEditor,
                             private juce::Timer
{
public:
    explicit MugenSpaceDrumEditor (MugenSpaceDrumProcessor&);
    ~MugenSpaceDrumEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    struct Section
    {
        juce::String title;
        int firstKnob = 0;
        int numKnobs  = 0;
        float units   = 1.0f;   // largura relativa do painel
        juce::Rectangle<int> bounds;
    };

    void setupKnob (Knob& knob, const char* parameterID, const juce::String& name);

    MugenSpaceDrumProcessor& processorRef;
    MugenLookAndFeel lookAndFeel;

    static constexpr int numKnobs = 11;
    std::array<Knob, numKnobs> knobs;
    std::array<Section, 5> sections;

    juce::ComboBox sampleSelector;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> sampleSelectorAttachment;

    juce::MidiKeyboardComponent keyboard;

    juce::Rectangle<int> ledBounds;
    float ledLevel = 0.0f;

    std::vector<juce::Point<float>> stars;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MugenSpaceDrumEditor)
};
