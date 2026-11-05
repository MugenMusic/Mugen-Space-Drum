#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "MugenLookAndFeel.h"

// ==============================================================================
//  PluginEditor — a janela do plugin.
//
//  Fila de cima:  INSTRUMENT · SAMPLE · MOTION · OUTPUT
//  Fila de baixo: DELAY · REVERB
//  Em baixo:      teclado no ecrã
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

    using SliderAttachment   = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment   = juce::AudioProcessorValueTreeState::ButtonAttachment;

    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<SliderAttachment> attachment;
    };

    struct Section
    {
        juce::String title;
        int row       = 0;
        int firstKnob = 0;
        int numKnobs  = 0;
        float units   = 1.0f;   // largura relativa do painel na sua fila
        juce::Rectangle<int> bounds;
        juce::Component* toolLeft  = nullptr;   // controlos opcionais no topo do painel
        juce::Component* toolRight = nullptr;
    };

    void setupKnob (Knob& knob, const char* parameterID, const juce::String& name);
    void setupSelector (juce::ComboBox& box, const juce::StringArray& items,
                        const char* parameterID, std::unique_ptr<ComboBoxAttachment>& attachment);
    void setupToggle (juce::TextButton& button, const juce::String& text,
                      const char* parameterID, std::unique_ptr<ButtonAttachment>& attachment);
    void layoutRow (int row, juce::Rectangle<int> area);

    MugenSpaceDrumProcessor& processorRef;
    MugenLookAndFeel lookAndFeel;

    static constexpr int numKnobs = 15;
    std::array<Knob, numKnobs> knobs;
    std::array<Section, 6> sections;

    juce::ComboBox sampleSelector, divisionSelector, reverbTypeSelector;
    std::unique_ptr<ComboBoxAttachment> sampleAttachment, divisionAttachment, reverbTypeAttachment;

    juce::TextButton syncButton, freezeButton;
    std::unique_ptr<ButtonAttachment> syncAttachment, freezeAttachment;

    juce::MidiKeyboardComponent keyboard;

    juce::Rectangle<int> ledBounds;
    float ledLevel = 0.0f;

    std::vector<juce::Point<float>> stars;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MugenSpaceDrumEditor)
};
