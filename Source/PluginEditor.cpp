#include "PluginEditor.h"

namespace
{
    constexpr int editorWidth  = 940;
    constexpr int editorHeight = 560;
}

MugenSpaceDrumEditor::MugenSpaceDrumEditor (MugenSpaceDrumProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&lookAndFeel);

    // Ordem dos knobs no ecrã
    setupKnob (knobs[0], ParamIDs::character,     "Character");
    setupKnob (knobs[1], ParamIDs::brightness,    "Brightness");
    setupKnob (knobs[2], ParamIDs::decay,         "Decay");
    setupKnob (knobs[3], ParamIDs::resonance,     "Resonance");
    setupKnob (knobs[4], ParamIDs::delayMix,      "Mix");
    setupKnob (knobs[5], ParamIDs::delayTime,     "Time");
    setupKnob (knobs[6], ParamIDs::delayFeedback, "Feedback");
    setupKnob (knobs[7], ParamIDs::reverbMix,     "Mix");
    setupKnob (knobs[8], ParamIDs::reverbSize,    "Size");
    setupKnob (knobs[9], ParamIDs::volume,        "Volume");

    sections[0] = { "INSTRUMENT", 0, 4, {} };
    sections[1] = { "DELAY",      4, 3, {} };
    sections[2] = { "REVERB",     7, 2, {} };
    sections[3] = { "OUTPUT",     9, 1, {} };

    // Teclado no ecrã para testar o som com o rato
    keyboard.setAvailableRange (36, 96);
    keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId,         juce::Colour (0xffd9d4f5));
    keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId,         juce::Colour (0xff1b1736));
    keyboard.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId,  juce::Colour (0xff3a3370));
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, MugenColours::blue.withAlpha (0.25f));
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId,    MugenColours::violet.withAlpha (0.75f));
    keyboard.setColour (juce::MidiKeyboardComponent::shadowColourId,            juce::Colours::transparentBlack);
    keyboard.setWantsKeyboardFocus (false);
    addAndMakeVisible (keyboard);

    // Pequeno céu estrelado (sempre igual, para não "piscar")
    juce::Random random (91);
    for (int i = 0; i < 70; ++i)
        stars.push_back ({ random.nextFloat() * (float) editorWidth, random.nextFloat() * 300.0f });

    setSize (editorWidth, editorHeight);
    startTimerHz (30);
}

MugenSpaceDrumEditor::~MugenSpaceDrumEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void MugenSpaceDrumEditor::setupKnob (Knob& knob, const char* parameterID, const juce::String& name)
{
    knob.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    knob.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 18);
    knob.slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                     juce::MathConstants<float>::pi * 2.75f, true);
    addAndMakeVisible (knob.slider);

    knob.label.setText (name, juce::dontSendNotification);
    knob.label.setJustificationType (juce::Justification::centred);
    knob.label.setFont (juce::Font (juce::FontOptions (13.0f)));
    addAndMakeVisible (knob.label);

    knob.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processorRef.parameters, parameterID, knob.slider);
}

void MugenSpaceDrumEditor::paint (juce::Graphics& g)
{
    // Fundo
    g.setGradientFill (juce::ColourGradient (MugenColours::backgroundTop, 0.0f, 0.0f,
                                             MugenColours::backgroundBottom, 0.0f, (float) getHeight(), false));
    g.fillAll();

    // Estrelas
    for (size_t i = 0; i < stars.size(); ++i)
    {
        const float size = (i % 7 == 0) ? 2.0f : 1.2f;
        g.setColour (juce::Colours::white.withAlpha ((i % 3 == 0) ? 0.35f : 0.15f));
        g.fillEllipse (stars[i].x, stars[i].y, size, size);
    }

    // Título
    g.setColour (MugenColours::text);
    g.setFont (juce::Font (juce::FontOptions (30.0f, juce::Font::bold)));
    g.drawText ("MUGEN", 28, 22, 200, 36, juce::Justification::centredLeft);

    g.setGradientFill (juce::ColourGradient (MugenColours::violet, 140.0f, 0.0f,
                                             MugenColours::blue, 420.0f, 0.0f, false));
    g.setFont (juce::Font (juce::FontOptions (30.0f)));
    g.drawText ("SPACE DRUM", 140, 22, 320, 36, juce::Justification::centredLeft);

    g.setColour (MugenColours::textDim);
    g.setFont (juce::Font (juce::FontOptions (12.0f)));
    g.drawText ("Enter another dimension.", 30, 56, 300, 18, juce::Justification::centredLeft);

    // Luz de atividade MIDI
    g.setColour (MugenColours::textDim);
    g.drawText ("MIDI", ledBounds.getX() - 40, ledBounds.getY() - 3, 34, 18, juce::Justification::centredRight);
    g.setColour (MugenColours::knobTrack);
    g.fillEllipse (ledBounds.toFloat());
    if (ledLevel > 0.01f)
    {
        g.setColour (MugenColours::blue.withAlpha (ledLevel * 0.35f));
        g.fillEllipse (ledBounds.toFloat().expanded (5.0f));
        g.setColour (MugenColours::blue.withAlpha (ledLevel));
        g.fillEllipse (ledBounds.toFloat());
    }

    // Painéis das secções
    for (const auto& section : sections)
    {
        const auto area = section.bounds.toFloat();
        g.setColour (MugenColours::panel);
        g.fillRoundedRectangle (area, 14.0f);
        g.setColour (MugenColours::panelOutline);
        g.drawRoundedRectangle (area.reduced (0.5f), 14.0f, 1.0f);

        g.setColour (MugenColours::violet.interpolatedWith (MugenColours::blue, 0.4f));
        g.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
        g.drawText (section.title, section.bounds.withHeight (34), juce::Justification::centred);
    }

    // Assinatura
    g.setColour (MugenColours::textDim);
    g.setFont (juce::Font (juce::FontOptions (12.0f)));
    g.drawText ("Made by Mugen  ·  v" JucePlugin_VersionString,
                getLocalBounds().withTrimmedTop (keyboard.getY() - 26).withHeight (20).reduced (28, 0),
                juce::Justification::centredRight);
}

void MugenSpaceDrumEditor::resized()
{
    auto area = getLocalBounds().reduced (20);

    // Cabeçalho
    auto header = area.removeFromTop (70);
    ledBounds = juce::Rectangle<int> (header.getRight() - 22, header.getY() + 12, 12, 12);

    // Teclado em baixo
    auto keyboardArea = area.removeFromBottom (110);
    keyboard.setBounds (keyboardArea);
    keyboard.setKeyWidth ((float) keyboardArea.getWidth() / 36.0f); // 36 teclas brancas (C2 a C7)

    area.removeFromBottom (34);

    // Painéis: larguras proporcionais ao número de knobs
    const int gap = 12;
    const int totalKnobs = numKnobs;
    const int usableWidth = area.getWidth() - gap * ((int) sections.size() - 1);

    for (size_t s = 0; s < sections.size(); ++s)
    {
        auto& section = sections[s];
        const int width = (s == sections.size() - 1) ? area.getWidth()
                                                     : usableWidth * section.numKnobs / totalKnobs;
        section.bounds = area.removeFromLeft (width);
        area.removeFromLeft (gap);

        auto inner = section.bounds.reduced (6).withTrimmedTop (30);
        const int knobWidth = inner.getWidth() / section.numKnobs;

        for (int k = 0; k < section.numKnobs; ++k)
        {
            auto& knob = knobs[(size_t) (section.firstKnob + k)];
            auto column = inner.removeFromLeft (knobWidth);
            knob.label.setBounds (column.removeFromBottom (22));
            const int size = juce::jmin (column.getWidth(), column.getHeight());
            knob.slider.setBounds (column.withSizeKeepingCentre (size, size));
        }
    }
}

void MugenSpaceDrumEditor::timerCallback()
{
    if (processorRef.midiActivity.exchange (false))
        ledLevel = 1.0f;
    else
        ledLevel *= 0.85f;

    repaint (ledBounds.expanded (8));
}
