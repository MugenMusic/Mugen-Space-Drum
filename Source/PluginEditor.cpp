#include "PluginEditor.h"

namespace
{
    using SliderAttachment   = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment   = juce::AudioProcessorValueTreeState::ButtonAttachment;

    // Áreas principais da janela (coordenadas de desenho, 1180 x 760)
    const juce::Rectangle<int> headerArea   { 0,   0,   1180, 62 };
    const juce::Rectangle<int> tabArea      { 20,  70,  900,  36 };
    const juce::Rectangle<int> stageArea    { 20,  114, 900,  446 };
    const juce::Rectangle<int> browserArea  { 936, 70,  224,  490 };
    const juce::Rectangle<int> footerArea   { 20,  568, 1140, 26 };
    const juce::Rectangle<int> keyboardArea { 20,  598, 1140, 142 };

    constexpr int columnWidth = 262;

    juce::Font font (float size, bool bold = false, float kerning = 0.0f)
    {
        return juce::Font (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain)
                               .withKerningFactor (kerning));
    }
}

// ==============================================================================
//  Teclado com as notas da escala marcadas
// ==============================================================================
class ScaleKeyboard : public juce::MidiKeyboardComponent
{
public:
    ScaleKeyboard (juce::MidiKeyboardState& keyState, MugenSpaceDrumProcessor& p)
        : juce::MidiKeyboardComponent (keyState, horizontalKeyboard), processor (p)
    {
        setAvailableRange (36, 96);
        setColour (whiteNoteColourId,           juce::Colour (0xffd9d4f5));
        setColour (blackNoteColourId,           juce::Colour (0xff1b1736));
        setColour (keySeparatorLineColourId,    juce::Colour (0xff3a3370));
        setColour (mouseOverKeyOverlayColourId, MugenColours::blue.withAlpha (0.25f));
        setColour (keyDownOverlayColourId,      MugenColours::violet.withAlpha (0.75f));
        setColour (shadowColourId,              juce::Colours::transparentBlack);
        setWantsKeyboardFocus (false);
    }

    void drawWhiteNote (int note, juce::Graphics& g, juce::Rectangle<float> area,
                        bool isDown, bool isOver, juce::Colour lineColour, juce::Colour textColour) override
    {
        juce::MidiKeyboardComponent::drawWhiteNote (note, g, area, isDown, isOver, lineColour, textColour);
        drawScaleMark (note, g, area);
    }

    void drawBlackNote (int note, juce::Graphics& g, juce::Rectangle<float> area,
                        bool isDown, bool isOver, juce::Colour noteFillColour) override
    {
        juce::MidiKeyboardComponent::drawBlackNote (note, g, area, isDown, isOver, noteFillColour);
        drawScaleMark (note, g, area);
    }

private:
    void drawScaleMark (int note, juce::Graphics& g, juce::Rectangle<float> area)
    {
        const int root  = juce::roundToInt (processor.parameters.getRawParameterValue (ParamIDs::scaleRoot)->load());
        const int scale = juce::roundToInt (processor.parameters.getRawParameterValue (ParamIDs::scaleType)->load());

        if (scale == 0 || ! ScaleEngine::isInScale (note, root, scale))   // Chromatic: sem marcas
            return;

        const bool isRoot = ((note - root) % 12 + 12) % 12 == 0;
        const float size = juce::jmin (area.getWidth() * 0.45f, 8.0f);
        const auto dot = juce::Rectangle<float> (size, size).withCentre ({ area.getCentreX(), area.getBottom() - size - 4.0f });

        g.setColour (isRoot ? MugenColours::blue : MugenColours::violet);
        g.fillEllipse (dot);
    }

    MugenSpaceDrumProcessor& processor;
};

// ==============================================================================
//  Browser de presets (lista por categorias)
// ==============================================================================
class PresetBrowser : public juce::Component,
                      private juce::ListBoxModel
{
public:
    explicit PresetBrowser (PresetManager& manager)
        : presets (manager)
    {
        list.setModel (this);
        list.setRowHeight (24);
        list.setColour (juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
        list.setColour (juce::ListBox::outlineColourId, juce::Colours::transparentBlack);
        list.getVerticalScrollBar().setColour (juce::ScrollBar::thumbColourId, MugenColours::violet.withAlpha (0.5f));
        addAndMakeVisible (list);
        rebuild();
    }

    std::function<void()> onPresetChosen;

    void rebuild()
    {
        rows.clear();
        const auto& entries = presets.getEntries();

        for (const auto& category : PresetManager::categoryOrder())
        {
            bool headerAdded = false;
            for (size_t i = 0; i < entries.size(); ++i)
            {
                if (entries[i].category != category)
                    continue;
                if (! headerAdded)
                {
                    rows.push_back ({ true, category.toUpperCase(), -1 });
                    headerAdded = true;
                }
                rows.push_back ({ false, entries[i].name, (int) i });
            }
        }

        list.updateContent();
        list.repaint();
    }

    void refreshHighlight() { list.repaint(); }

    void paint (juce::Graphics& g) override
    {
        auto area = getLocalBounds().toFloat();
        g.setColour (juce::Colour (0x88100d22));
        g.fillRoundedRectangle (area, 12.0f);
        g.setColour (MugenColours::panelOutline);
        g.drawRoundedRectangle (area.reduced (0.5f), 12.0f, 1.0f);

        g.setColour (MugenColours::text);
        g.setFont (font (12.0f, true, 0.2f));
        g.drawText ("BROWSER", getLocalBounds().removeFromTop (34).withTrimmedLeft (16), juce::Justification::centredLeft);
    }

    void resized() override
    {
        list.setBounds (getLocalBounds().reduced (8).withTrimmedTop (28));
    }

private:
    struct Row
    {
        bool isHeader;
        juce::String text;
        int entryIndex;
    };

    int getNumRows() override { return (int) rows.size(); }

    void paintListBoxItem (int rowNumber, juce::Graphics& g, int width, int height, bool) override
    {
        if (rowNumber < 0 || rowNumber >= (int) rows.size())
            return;

        const auto& row = rows[(size_t) rowNumber];

        if (row.isHeader)
        {
            g.setColour (MugenColours::violet.interpolatedWith (MugenColours::blue, 0.4f));
            g.setFont (font (11.0f, true, 0.15f));
            g.drawText (row.text, 8, 0, width - 8, height, juce::Justification::bottomLeft);
            return;
        }

        const bool isCurrent = presets.getCurrentIndex() == row.entryIndex;
        if (isCurrent)
        {
            g.setGradientFill (juce::ColourGradient (MugenColours::violet.withAlpha (0.45f), 0.0f, 0.0f,
                                                     MugenColours::blue.withAlpha (0.15f), (float) width, 0.0f, false));
            g.fillRoundedRectangle (2.0f, 2.0f, (float) width - 4.0f, (float) height - 4.0f, 6.0f);
        }

        g.setColour (isCurrent ? juce::Colours::white : MugenColours::text.withAlpha (0.8f));
        g.setFont (font (13.0f));
        g.drawText (row.text, 18, 0, width - 20, height, juce::Justification::centredLeft);
    }

    void listBoxItemClicked (int rowNumber, const juce::MouseEvent&) override
    {
        if (rowNumber < 0 || rowNumber >= (int) rows.size() || rows[(size_t) rowNumber].isHeader)
            return;

        presets.load (rows[(size_t) rowNumber].entryIndex);
        list.repaint();
        if (onPresetChosen)
            onPresetChosen();
    }

    PresetManager& presets;
    juce::ListBox list;
    std::vector<Row> rows;
};

// ==============================================================================
//  Página de um separador: uma coluna de controlos à esquerda e outra à direita
// ==============================================================================
class TabPage : public juce::Component
{
public:
    struct Column
    {
        juce::String title, caption;
        std::vector<std::vector<juce::Component*>> toolRows;   // seletores / botões (linhas)
        std::vector<std::pair<juce::Slider*, juce::Label*>> knobs;
        juce::Rectangle<int> bounds;
    };

    Column left, right;

    void paint (juce::Graphics& g) override
    {
        for (auto* column : { &left, &right })
        {
            const auto area = column->bounds.toFloat();
            g.setColour (MugenColours::panel);
            g.fillRoundedRectangle (area, 14.0f);
            g.setColour (MugenColours::panelOutline);
            g.drawRoundedRectangle (area.reduced (0.5f), 14.0f, 1.0f);

            g.setColour (MugenColours::violet.interpolatedWith (MugenColours::blue, 0.4f));
            g.setFont (font (12.0f, true, 0.2f));
            g.drawText (column->title, column->bounds.withHeight (30).withTrimmedTop (6), juce::Justification::centred);

            g.setColour (MugenColours::textDim);
            g.setFont (font (11.0f));
            g.drawText (column->caption, column->bounds.withTrimmedTop (28).withHeight (16), juce::Justification::centred);
        }
    }

    void resized() override
    {
        left.bounds  = getLocalBounds().removeFromLeft (columnWidth);
        right.bounds = getLocalBounds().removeFromRight (columnWidth);
        layoutColumn (left);
        layoutColumn (right);
    }

private:
    static void layoutColumn (Column& column)
    {
        auto area = column.bounds.reduced (14).withTrimmedTop (40);

        for (auto& row : column.toolRows)
        {
            auto line = area.removeFromTop (28);
            const int w = line.getWidth() / juce::jmax (1, (int) row.size());
            for (auto* c : row)
                c->setBounds (line.removeFromLeft (w).reduced (3, 0));
            area.removeFromTop (8);
        }

        const int count = (int) column.knobs.size();
        if (count == 0)
            return;

        const int perRow = count == 1 ? 1 : 2;
        const int rowsNeeded = (count + perRow - 1) / perRow;
        const int rowHeight = area.getHeight() / rowsNeeded;
        const int maxSize = count == 1 ? 170 : 118;

        for (int r = 0; r < rowsNeeded; ++r)
        {
            auto rowArea = area.removeFromTop (rowHeight);
            const int inThisRow = juce::jmin (perRow, count - r * perRow);
            const int cellWidth = rowArea.getWidth() / perRow;
            // Centrar uma linha incompleta
            rowArea = rowArea.withSizeKeepingCentre (cellWidth * inThisRow, rowArea.getHeight());

            for (int k = 0; k < inThisRow; ++k)
            {
                auto [slider, label] = column.knobs[(size_t) (r * perRow + k)];
                auto cell = rowArea.removeFromLeft (cellWidth);
                label->setBounds (cell.removeFromBottom (20));
                const int size = juce::jmin (maxSize, juce::jmin (cell.getWidth(), cell.getHeight()));
                slider->setBounds (cell.withSizeKeepingCentre (size, size));
            }
        }
    }
};

// ==============================================================================
//  MainView — tudo o que se vê, desenhado em 1180 x 760 e depois escalado
// ==============================================================================
class MugenSpaceDrumEditor::MainView : public juce::Component,
                                       private juce::Timer
{
public:
    explicit MainView (MugenSpaceDrumProcessor& p)
        : processor (p),
          orb (p),
          browser (p.presetManager),
          keyboard (p.keyboardState, p)
    {
        // --- Knobs ------------------------------------------------------------------
        makeKnob (character,   ParamIDs::character,     "Character");
        makeKnob (brightness,  ParamIDs::brightness,    "Brightness");
        makeKnob (blend,       ParamIDs::blend,         "Blend");
        makeKnob (resonance,   ParamIDs::resonance,     "Resonance");
        makeKnob (decay,       ParamIDs::decay,         "Decay");
        makeKnob (delayMix,    ParamIDs::delayMix,      "Mix");
        makeKnob (delayTime,   ParamIDs::delayTime,     "Time");
        makeKnob (feedback,    ParamIDs::delayFeedback, "Feedback");
        makeKnob (reverbMix,   ParamIDs::reverbMix,     "Mix");
        makeKnob (reverbSize,  ParamIDs::reverbSize,    "Size");
        makeKnob (shimmer,     ParamIDs::shimmer,       "Shimmer");
        makeKnob (depth,       ParamIDs::motionDepth,   "Depth");
        makeKnob (rate,        ParamIDs::motionRate,    "Rate");
        makeKnob (width,       ParamIDs::width,         "Width");
        makeKnob (volume,      ParamIDs::volume,        "Volume");
        makeKnob (octave,      ParamIDs::octave,        "Octave");
        makeKnob (tune,        ParamIDs::fineTune,      "Tune");

        // --- Seletores e botões ---------------------------------------------------------
        makeSelector (sampleSelector,   SampleBank::typeNames(),     ParamIDs::sampleType);
        makeSelector (divisionSelector, DelayDivisions::names(),     ParamIDs::delayDivision);
        makeSelector (reverbSelector,   ReverbEngine::typeNames(),   ParamIDs::reverbType);
        makeSelector (rootSelector,     ScaleEngine::rootNames(),    ParamIDs::scaleRoot);
        makeSelector (scaleSelector,    ScaleEngine::scaleNames(),   ParamIDs::scaleType);
        makeSelector (mappingSelector,  ScaleEngine::mappingNames(), ParamIDs::scaleMapping);
        makeToggle (syncButton,   "SYNC",   ParamIDs::delaySync);
        makeToggle (freezeButton, "FREEZE", ParamIDs::freeze);

        // --- Páginas dos separadores -----------------------------------------------------
        setupPages();

        const juce::StringArray tabNames { "INSTRUMENT", "RESONANCE", "AMBIENCE", "MOTION", "SCALE & TUNING" };
        for (int i = 0; i < tabNames.size(); ++i)
        {
            auto& tab = tabs[(size_t) i];
            tab.setButtonText (tabNames[i]);
            tab.setClickingTogglesState (true);
            tab.setRadioGroupId (1001);
            tab.getProperties().set ("tab", true);
            tab.onClick = [this, i] { if (tabs[(size_t) i].getToggleState()) showPage (i); };
            addAndMakeVisible (tab);
        }

        addAndMakeVisible (orb);

        // --- Barra de presets ---------------------------------------------------------------
        for (auto* b : { &previousButton, &nextButton, &saveButton })
            addAndMakeVisible (*b);
        previousButton.setButtonText ("<");
        nextButton.setButtonText (">");
        saveButton.setButtonText ("SAVE");
        presetNameButton.getProperties().set ("presetName", true);
        addAndMakeVisible (presetNameButton);

        previousButton.onClick   = [this] { processor.presetManager.loadPrevious(); presetChanged(); };
        nextButton.onClick       = [this] { processor.presetManager.loadNext();     presetChanged(); };
        presetNameButton.onClick = [this] { showPresetMenu(); };
        saveButton.onClick       = [this] { savePreset(); };

        browser.onPresetChosen = [this] { presetChanged(); };
        addAndMakeVisible (browser);
        addAndMakeVisible (keyboard);

        // Céu estrelado (sempre igual)
        juce::Random random (91);
        for (int i = 0; i < 140; ++i)
            stars.push_back ({ random.nextFloat() * 1180.0f, random.nextFloat() * 600.0f });

        const int savedTab = juce::jlimit (0, 4, (int) processor.parameters.state.getProperty ("uiTab", 0));
        tabs[(size_t) savedTab].setToggleState (true, juce::dontSendNotification);
        showPage (savedTab);

        presetChanged();
        startTimerHz (30);
    }

    ~MainView() override { stopTimer(); }

    void paint (juce::Graphics& g) override
    {
        // Fundo: céu profundo com nebulosas suaves
        g.setGradientFill (juce::ColourGradient (MugenColours::backgroundTop, 0.0f, 0.0f,
                                                 MugenColours::backgroundBottom, 0.0f, (float) getHeight(), false));
        g.fillAll();

        g.setGradientFill (juce::ColourGradient (MugenColours::violet.withAlpha (0.18f), 220.0f, 120.0f,
                                                 MugenColours::violet.withAlpha (0.0f), 620.0f, 420.0f, true));
        g.fillRect (getLocalBounds());
        g.setGradientFill (juce::ColourGradient (MugenColours::blue.withAlpha (0.10f), 900.0f, 300.0f,
                                                 MugenColours::blue.withAlpha (0.0f), 1180.0f, 650.0f, true));
        g.fillRect (getLocalBounds());

        for (size_t i = 0; i < stars.size(); ++i)
        {
            const float size = (i % 9 == 0) ? 2.0f : 1.1f;
            g.setColour (juce::Colours::white.withAlpha ((i % 4 == 0) ? 0.35f : 0.12f));
            g.fillEllipse (stars[i].x, stars[i].y, size, size);
        }

        // Cabeçalho
        g.setColour (juce::Colour (0xcc0a0816));
        g.fillRect (headerArea);
        juce::ColourGradient line (MugenColours::violet.withAlpha (0.0f), 0.0f, 62.0f,
                                   MugenColours::blue.withAlpha (0.0f), 1180.0f, 62.0f, false);
        line.addColour (0.5, MugenColours::violet.withAlpha (0.8f));
        g.setGradientFill (line);
        g.fillRect (0, 61, 1180, 1);

        g.setColour (MugenColours::text);
        g.setFont (font (24.0f, true, 0.35f));
        g.drawText ("MUGEN", 24, 8, 180, 30, juce::Justification::centredLeft);
        g.setGradientFill (juce::ColourGradient (MugenColours::violet, 24.0f, 0.0f, MugenColours::blue, 220.0f, 0.0f, false));
        g.setFont (font (11.0f, false, 0.55f));
        g.drawText ("SPACE DRUM", 25, 36, 220, 16, juce::Justification::centredLeft);

        // Luz MIDI
        g.setColour (MugenColours::textDim);
        g.setFont (font (11.0f));
        g.drawText ("MIDI", ledBounds.getX() - 38, ledBounds.getY() - 3, 32, 18, juce::Justification::centredRight);
        g.setColour (MugenColours::knobTrack);
        g.fillEllipse (ledBounds.toFloat());
        if (ledLevel > 0.01f)
        {
            g.setColour (MugenColours::blue.withAlpha (ledLevel * 0.35f));
            g.fillEllipse (ledBounds.toFloat().expanded (5.0f));
            g.setColour (MugenColours::blue.withAlpha (ledLevel));
            g.fillEllipse (ledBounds.toFloat());
        }

        // Rodapé: assinatura e legenda do teclado
        g.setColour (MugenColours::textDim);
        g.setFont (font (11.0f));
        g.drawText ("Made by Mugen  |  v" JucePlugin_VersionString, footerArea, juce::Justification::centredLeft);

        const auto legend = footerArea.withTrimmedLeft (footerArea.getWidth() - 220);
        g.setColour (MugenColours::violet);
        g.fillEllipse ((float) legend.getX(), (float) legend.getCentreY() - 4.0f, 8.0f, 8.0f);
        g.setColour (MugenColours::textDim);
        g.drawText ("scale note", legend.withTrimmedLeft (14).withWidth (90), juce::Justification::centredLeft);
        g.setColour (MugenColours::blue);
        g.fillEllipse ((float) legend.getX() + 110.0f, (float) legend.getCentreY() - 4.0f, 8.0f, 8.0f);
        g.setColour (MugenColours::textDim);
        g.drawText ("root", legend.withTrimmedLeft (124).withWidth (80), juce::Justification::centredLeft);
    }

    void resized() override
    {
        // Barra de presets ao centro do cabeçalho
        juce::Rectangle<int> bar (400, 14, 420, 34);
        previousButton.setBounds (bar.removeFromLeft (34));
        bar.removeFromLeft (6);
        saveButton.setBounds (bar.removeFromRight (64));
        bar.removeFromRight (6);
        nextButton.setBounds (bar.removeFromRight (34));
        bar.removeFromRight (6);
        presetNameButton.setBounds (bar);

        ledBounds = { 1146, 25, 12, 12 };

        // Separadores
        auto tabRow = tabArea;
        const int tabWidth = tabRow.getWidth() / (int) tabs.size();
        for (auto& tab : tabs)
            tab.setBounds (tabRow.removeFromLeft (tabWidth));

        for (auto& page : pages)
            page.setBounds (stageArea);

        orb.setBounds (stageArea.reduced (columnWidth + 6, 0));
        browser.setBounds (browserArea);

        keyboard.setBounds (keyboardArea);
        keyboard.setKeyWidth ((float) keyboardArea.getWidth() / 36.0f);   // C2 a C7
    }

private:
    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<SliderAttachment> attachment;
    };

    struct Selector
    {
        juce::ComboBox box;
        std::unique_ptr<ComboBoxAttachment> attachment;
    };

    struct Toggle
    {
        juce::TextButton button;
        std::unique_ptr<ButtonAttachment> attachment;
    };

    void makeKnob (Knob& knob, const char* id, const juce::String& name)
    {
        knob.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        knob.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 90, 18);
        knob.slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                         juce::MathConstants<float>::pi * 2.75f, true);
        knob.label.setText (name, juce::dontSendNotification);
        knob.label.setJustificationType (juce::Justification::centred);
        knob.label.setFont (font (13.0f));
        knob.attachment = std::make_unique<SliderAttachment> (processor.parameters, id, knob.slider);
    }

    void makeSelector (Selector& s, const juce::StringArray& items, const char* id)
    {
        s.box.addItemList (items, 1);
        s.box.setJustificationType (juce::Justification::centred);
        s.attachment = std::make_unique<ComboBoxAttachment> (processor.parameters, id, s.box);
    }

    void makeToggle (Toggle& t, const juce::String& text, const char* id)
    {
        t.button.setButtonText (text);
        t.button.setClickingTogglesState (true);
        t.attachment = std::make_unique<ButtonAttachment> (processor.parameters, id, t.button);
    }

    void addColumn (TabPage& page, TabPage::Column& column, const juce::String& title, const juce::String& caption,
                    std::vector<std::vector<juce::Component*>> tools, std::vector<Knob*> knobList)
    {
        column.title = title;
        column.caption = caption;
        column.toolRows = tools;
        for (auto& row : tools)
            for (auto* c : row)
                page.addAndMakeVisible (c);
        for (auto* k : knobList)
        {
            page.addAndMakeVisible (k->slider);
            page.addAndMakeVisible (k->label);
            column.knobs.push_back ({ &k->slider, &k->label });
        }
    }

    void setupPages()
    {
        auto& instrument = pages[0];
        addColumn (instrument, instrument.left, "BODY & MALLET", "Instrument body and mallet hardness",
                   {}, { &character, &brightness });
        addColumn (instrument, instrument.right, "ORGANIC SAMPLE", "Sample generated inside the plugin",
                   { { &sampleSelector.box } }, { &blend });

        auto& resonancePage = pages[1];
        addColumn (resonancePage, resonancePage.left, "RESONANCE", "Ethereal beating of the harmonics",
                   {}, { &resonance });
        addColumn (resonancePage, resonancePage.right, "SUSTAIN", "How long each note rings",
                   {}, { &decay });

        auto& ambience = pages[2];
        addColumn (ambience, ambience.left, "DELAY", "Warm ping-pong echoes",
                   { { &syncButton.button, &divisionSelector.box } }, { &delayMix, &delayTime, &feedback });
        addColumn (ambience, ambience.right, "REVERB", "Room, Hall, Cathedral, Deep Space",
                   { { &reverbSelector.box, &freezeButton.button } }, { &reverbMix, &reverbSize, &shimmer });

        auto& motion = pages[3];
        addColumn (motion, motion.left, "MOVEMENT", "Slow hypnotic motion",
                   {}, { &depth, &rate });
        addColumn (motion, motion.right, "OUTPUT", "Stereo width and level",
                   {}, { &width, &volume });

        auto& scalePage = pages[4];
        addColumn (scalePage, scalePage.left, "SCALE", "Every key plays a note of the scale",
                   { { &rootSelector.box, &mappingSelector.box }, { &scaleSelector.box } }, {});
        addColumn (scalePage, scalePage.right, "TUNING", "Octave shift and fine tune",
                   {}, { &octave, &tune });

        for (auto& page : pages)
            addChildComponent (page);
    }

    void showPage (int index)
    {
        for (size_t i = 0; i < pages.size(); ++i)
            pages[i].setVisible ((int) i == index);
        processor.parameters.state.setProperty ("uiTab", index, nullptr);
    }

    void presetChanged()
    {
        lastPresetName = processor.presetManager.getCurrentName();
        presetNameButton.setButtonText (lastPresetName);
        browser.refreshHighlight();
        keyboard.repaint();
    }

    void showPresetMenu()
    {
        juce::PopupMenu menu;
        const auto& entries = processor.presetManager.getEntries();
        const int current = processor.presetManager.getCurrentIndex();

        for (const auto& category : PresetManager::categoryOrder())
        {
            juce::PopupMenu sub;
            for (size_t i = 0; i < entries.size(); ++i)
                if (entries[i].category == category)
                    sub.addItem ((int) i + 1, entries[i].name, true, (int) i == current);
            if (sub.getNumItems() > 0)
                menu.addSubMenu (category, sub);
        }

        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (presetNameButton),
                            [this] (int result)
                            {
                                if (result > 0)
                                {
                                    processor.presetManager.load (result - 1);
                                    presetChanged();
                                }
                            });
    }

    void savePreset()
    {
        auto folder = PresetManager::getUserPresetFolder();
        folder.createDirectory();

        chooser = std::make_unique<juce::FileChooser> ("Save Mugen Space Drum preset",
                                                       folder.getChildFile ("My Preset" + juce::String (PresetManager::fileExtension)),
                                                       juce::String ("*") + PresetManager::fileExtension);

        chooser->launchAsync (juce::FileBrowserComponent::saveMode
                                  | juce::FileBrowserComponent::canSelectFiles
                                  | juce::FileBrowserComponent::warnAboutOverwriting,
                              [this] (const juce::FileChooser& fc)
                              {
                                  const auto file = fc.getResult();
                                  if (file == juce::File())
                                      return;
                                  if (processor.presetManager.saveUserPreset (file))
                                  {
                                      browser.rebuild();
                                      presetChanged();
                                  }
                              });
    }

    void timerCallback() override
    {
        if (processor.midiActivity.exchange (false))
            ledLevel = 1.0f;
        else
            ledLevel *= 0.85f;
        repaint (ledBounds.expanded (8));

        // Com o Sync ligado, o tempo vem do BPM do DAW: o knob Time fica inativo.
        const bool synced = syncButton.button.getToggleState();
        delayTime.slider.setEnabled (! synced);
        delayTime.slider.setAlpha (synced ? 0.35f : 1.0f);
        divisionSelector.box.setEnabled (synced);
        divisionSelector.box.setAlpha (synced ? 1.0f : 0.45f);

        // O nome do preset pode mudar por fora (DAW, abrir projeto...)
        if (processor.presetManager.getCurrentName() != lastPresetName)
            presetChanged();

        // Marcas da escala no teclado
        const int signature = rootSelector.box.getSelectedItemIndex() * 1000 + scaleSelector.box.getSelectedItemIndex();
        if (signature != lastScaleSignature)
        {
            lastScaleSignature = signature;
            keyboard.repaint();
        }
    }

    MugenSpaceDrumProcessor& processor;

    Knob character, brightness, blend, resonance, decay,
         delayMix, delayTime, feedback, reverbMix, reverbSize, shimmer,
         depth, rate, width, volume, octave, tune;

    Selector sampleSelector, divisionSelector, reverbSelector, rootSelector, scaleSelector, mappingSelector;
    Toggle syncButton, freezeButton;

    std::array<TabPage, 5> pages;
    std::array<juce::TextButton, 5> tabs;

    OrbVisualizer orb;
    PresetBrowser browser;
    ScaleKeyboard keyboard;

    juce::TextButton previousButton, nextButton, saveButton, presetNameButton;
    std::unique_ptr<juce::FileChooser> chooser;

    juce::Rectangle<int> ledBounds;
    float ledLevel = 0.0f;
    juce::String lastPresetName;
    int lastScaleSignature = -1;
    std::vector<juce::Point<float>> stars;
};

// ==============================================================================
MugenSpaceDrumEditor::MugenSpaceDrumEditor (MugenSpaceDrumProcessor& p)
    : AudioProcessorEditor (&p)
{
    setLookAndFeel (&lookAndFeel);

    view = std::make_unique<MainView> (p);
    addAndMakeVisible (*view);

    // Janela redimensionável, sempre com as mesmas proporções.
    setResizable (true, true);
    setResizeLimits (designWidth * 6 / 10, designHeight * 6 / 10, designWidth * 3 / 2, designHeight * 3 / 2);
    if (auto* sizeLimits = getConstrainer())
        sizeLimits->setFixedAspectRatio ((double) designWidth / (double) designHeight);

    setSize (designWidth, designHeight);
}

MugenSpaceDrumEditor::~MugenSpaceDrumEditor()
{
    view.reset();
    setLookAndFeel (nullptr);
}

void MugenSpaceDrumEditor::resized()
{
    const float scale = (float) getWidth() / (float) designWidth;
    view->setTransform (juce::AffineTransform::scale (scale));
    view->setBounds (0, 0, designWidth, designHeight);
}
