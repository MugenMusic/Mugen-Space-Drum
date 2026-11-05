#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "MugenLookAndFeel.h"

// ==============================================================================
//  PluginEditor — a janela do plugin.
//
//  Fila de cima:  INSTRUMENT · SAMPLE · MOTION · OUTPUT
//  Fila de baixo: DELAY · REVERB · SCALE
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
        juce::Component* toolBelow = nullptr;   // segunda linha de controlos (opcional)
    };

    /** Teclado que marca as notas da escala: ponto violeta = nota da escala, azul = nota raiz. */
    class ScaleKeyboard : public juce::MidiKeyboardComponent
    {
    public:
        ScaleKeyboard (juce::MidiKeyboardState& keyState, MugenSpaceDrumProcessor& p)
            : juce::MidiKeyboardComponent (keyState, horizontalKeyboard), processor (p) {}

        void drawWhiteNote (int note, juce::Graphics& g, juce::Rectangle<float> area,
                            bool isDown, bool isOver, juce::Colour lineColour, juce::Colour textColour) override;
        void drawBlackNote (int note, juce::Graphics& g, juce::Rectangle<float> area,
                            bool isDown, bool isOver, juce::Colour noteFillColour) override;

    private:
        void drawScaleMark (int note, juce::Graphics& g, juce::Rectangle<float> area);
        MugenSpaceDrumProcessor& processor;
    };

    void setupKnob (Knob& knob, const char* parameterID, const juce::String& name);
    void setupSelector (juce::ComboBox& box, const juce::StringArray& items,
                        const char* parameterID, std::unique_ptr<ComboBoxAttachment>& attachment);
    void setupToggle (juce::TextButton& button, const juce::String& text,
                      const char* parameterID, std::unique_ptr<ButtonAttachment>& attachment);
    void layoutRow (int row, juce::Rectangle<int> area);

    MugenSpaceDrumProcessor& processorRef;
    MugenLookAndFeel lookAndFeel;

    static constexpr int numKnobs = 17;
    std::array<Knob, numKnobs> knobs;
    std::array<Section, 7> sections;

    juce::ComboBox sampleSelector, divisionSelector, reverbTypeSelector;
    std::unique_ptr<ComboBoxAttachment> sampleAttachment, divisionAttachment, reverbTypeAttachment;

    juce::ComboBox rootSelector, scaleSelector, mappingSelector;
    std::unique_ptr<ComboBoxAttachment> rootAttachment, scaleAttachment, mappingAttachment;
    int lastScaleSignature = -1;

    juce::TextButton syncButton, freezeButton;
    std::unique_ptr<ButtonAttachment> syncAttachment, freezeAttachment;

    ScaleKeyboard keyboard;

    juce::Rectangle<int> ledBounds;
    float ledLevel = 0.0f;

    std::vector<juce::Point<float>> stars;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MugenSpaceDrumEditor)
};
