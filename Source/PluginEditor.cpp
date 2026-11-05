#include "PluginEditor.h"

namespace
{
    constexpr int editorWidth  = 1040;
    constexpr int editorHeight = 790;
    constexpr int maxKnobSize  = 104;

    // Índices dos knobs (para os ligar/desligar consoante o Sync)
    constexpr int delayTimeKnob = 10;
}

MugenSpaceDrumEditor::MugenSpaceDrumEditor (MugenSpaceDrumProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&lookAndFeel);

    // Ordem dos knobs no ecrã
    setupKnob (knobs[0],  ParamIDs::character,     "Character");
    setupKnob (knobs[1],  ParamIDs::brightness,    "Brightness");
    setupKnob (knobs[2],  ParamIDs::decay,         "Decay");
    setupKnob (knobs[3],  ParamIDs::resonance,     "Resonance");
    setupKnob (knobs[4],  ParamIDs::blend,         "Blend");
    setupKnob (knobs[5],  ParamIDs::motionDepth,   "Depth");
    setupKnob (knobs[6],  ParamIDs::motionRate,    "Rate");
    setupKnob (knobs[7],  ParamIDs::width,         "Width");
    setupKnob (knobs[8],  ParamIDs::volume,        "Volume");
    setupKnob (knobs[9],  ParamIDs::delayMix,      "Mix");
    setupKnob (knobs[10], ParamIDs::delayTime,     "Time");
    setupKnob (knobs[11], ParamIDs::delayFeedback, "Feedback");
    setupKnob (knobs[12], ParamIDs::reverbMix,     "Mix");
    setupKnob (knobs[13], ParamIDs::reverbSize,    "Size");
    setupKnob (knobs[14], ParamIDs::shimmer,       "Shimmer");

    // Seletores e botões
    setupSelector (sampleSelector,     SampleBank::typeNames(),     ParamIDs::sampleType,    sampleAttachment);
    setupSelector (divisionSelector,   DelayDivisions::names(),     ParamIDs::delayDivision, divisionAttachment);
    setupSelector (reverbTypeSelector, ReverbEngine::typeNames(),   ParamIDs::reverbType,    reverbTypeAttachment);
    setupToggle (syncButton,   "SYNC",   ParamIDs::delaySync, syncAttachment);
    setupToggle (freezeButton, "FREEZE", ParamIDs::freeze,    freezeAttachment);

    //            título        fila  1º knob  nº  largura  limites  topo esq.            topo dir.
    sections[0] = { "INSTRUMENT", 0,   0,      4,  4.0f,    {},      nullptr,             nullptr };
    sections[1] = { "SAMPLE",     0,   4,      1,  1.6f,    {},      &sampleSelector,     nullptr };
    sections[2] = { "MOTION",     0,   5,      2,  2.0f,    {},      nullptr,             nullptr };
    sections[3] = { "OUTPUT",     0,   7,      2,  2.0f,    {},      nullptr,             nullptr };
    sections[4] = { "DELAY",      1,   9,      3,  3.0f,    {},      &syncButton,         &divisionSelector };
    sections[5] = { "REVERB",     1,   12,     3,  3.0f,    {},      &reverbTypeSelector, &freezeButton };

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

void MugenSpaceDrumEditor::setupSelector (juce::ComboBox& box, const juce::StringArray& items,
                                          const char* parameterID, std::unique_ptr<ComboBoxAttachment>& attachment)
{
    box.addItemList (items, 1);
    box.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (box);
    attachment = std::make_unique<ComboBoxAttachment> (processorRef.parameters, parameterID, box);
}

void MugenSpaceDrumEditor::setupToggle (juce::TextButton& button, const juce::String& text,
                                        const char* parameterID, std::unique_ptr<ButtonAttachment>& attachment)
{
    button.setButtonText (text);
    button.setClickingTogglesState (true);
    addAndMakeVisible (button);
    attachment = std::make_unique<ButtonAttachment> (processorRef.parameters, parameterID, button);
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

    // Duas filas de painéis
    const int rowGap = 12;
    auto topRow = area.removeFromTop ((area.getHeight() - rowGap) / 2);
    area.removeFromTop (rowGap);

    layoutRow (0, topRow);
    layoutRow (1, area);
}

void MugenSpaceDrumEditor::layoutRow (int row, juce::Rectangle<int> area)
{
    const int gap = 12;
    float totalUnits = 0.0f;
    int count = 0;
    for (const auto& section : sections)
    {
        if (section.row != row)
            continue;
        totalUnits += section.units;
        ++count;
    }

    const int usableWidth = area.getWidth() - gap * (count - 1);
    int placed = 0;

    for (auto& section : sections)
    {
        if (section.row != row)
            continue;

        const bool last = (++placed == count);
        const int width = last ? area.getWidth()
                               : juce::roundToInt ((float) usableWidth * section.units / totalUnits);
        section.bounds = area.removeFromLeft (width);
        area.removeFromLeft (gap);

        auto inner = section.bounds.reduced (8).withTrimmedTop (28);

        // Controlos no topo do painel (seletores e botões)
        if (section.toolLeft != nullptr || section.toolRight != nullptr)
        {
            auto tools = inner.removeFromTop (26);
            if (section.toolLeft != nullptr && section.toolRight != nullptr)
            {
                section.toolLeft->setBounds (tools.removeFromLeft (tools.getWidth() / 2).reduced (6, 0));
                section.toolRight->setBounds (tools.reduced (6, 0));
            }
            else if (section.toolLeft != nullptr)
            {
                section.toolLeft->setBounds (tools.reduced (6, 0));
            }
            inner.removeFromTop (6);
        }

        const int knobWidth = inner.getWidth() / section.numKnobs;

        for (int k = 0; k < section.numKnobs; ++k)
        {
            auto& knob = knobs[(size_t) (section.firstKnob + k)];
            auto column = inner.removeFromLeft (knobWidth);
            knob.label.setBounds (column.removeFromBottom (22));
            const int size = juce::jmin (maxKnobSize, juce::jmin (column.getWidth(), column.getHeight()));
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

    // Com o Sync ligado, o tempo vem do BPM do DAW: o knob Time fica inativo.
    const bool synced = syncButton.getToggleState();
    knobs[delayTimeKnob].slider.setEnabled (! synced);
    knobs[delayTimeKnob].slider.setAlpha (synced ? 0.35f : 1.0f);
    divisionSelector.setEnabled (synced);
    divisionSelector.setAlpha (synced ? 1.0f : 0.45f);

    repaint (ledBounds.expanded (8));
}
