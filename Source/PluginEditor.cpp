#include "PluginEditor.h"

namespace
{
    using SliderAttachment   = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment   = juce::AudioProcessorValueTreeState::ButtonAttachment;

    // --- Mapa da janela (coordenadas de desenho, 1280 x 800) ----------------------
    const juce::Rectangle<int> topBarArea      { 0,    0,   1280, 40 };
    const juce::Rectangle<int> sidebarArea     { 0,    40,  264,  760 };
    const juce::Rectangle<int> headerBandArea  { 264,  40,  1016, 118 };
    const juce::Rectangle<int> patchDisplayArea{ 562,  106, 420,  40 };
    const juce::Rectangle<int> tabRowArea      { 397,  166, 750,  34 };
    const juce::Rectangle<int> tuningStripArea { 276,  208, 96,   428 };
    const juce::Rectangle<int> leftPanelArea   { 384,  208, 246,  428 };
    const juce::Rectangle<int> screenArea      { 642,  208, 264,  428 };
    const juce::Rectangle<int> rightPanelArea  { 918,  208, 246,  428 };
    const juce::Rectangle<int> outputStripArea { 1176, 208, 92,   428 };
    const juce::Rectangle<int> macroArea       { 276,  644, 992,  56 };
    const juce::Rectangle<int> keyboardArea    { 276,  708, 992,  82 };

    juce::Font font (float size, bool bold = false, float kerning = 0.0f)
    {
        return MugenLookAndFeel::uiFont (size, bold, kerning);
    }

    /** Painel com cabeçalho (título com pequeno triângulo) — o estilo base da interface. */
    void drawPanel (juce::Graphics& g, juce::Rectangle<int> bounds, const juce::String& title, bool screen = false)
    {
        const auto area = bounds.toFloat();
        g.setColour (screen ? MugenColours::screen : MugenColours::panel);
        g.fillRoundedRectangle (area, 10.0f);

        auto header = area.withHeight (30.0f);
        juce::Path headerShape;
        headerShape.addRoundedRectangle (header.getX(), header.getY(), header.getWidth(), header.getHeight(),
                                         10.0f, 10.0f, true, true, false, false);
        g.setColour (screen ? juce::Colour (0xff0e0c1c) : MugenColours::panelHeader);
        g.fillPath (headerShape);

        g.setColour (MugenColours::panelOutline);
        g.drawRoundedRectangle (area.reduced (0.5f), 10.0f, 1.0f);
        g.fillRect (header.withTop (header.getBottom() - 1.0f).reduced (1.0f, 0.0f));

        // Título com triângulo
        const auto titleFont = font (12.0f, true, 0.18f);
        g.setFont (titleFont);
        const float textWidth = juce::GlyphArrangement::getStringWidth (titleFont, title);
        const float startX = header.getCentreX() - (textWidth + 14.0f) * 0.5f;

        juce::Path triangle;
        triangle.addTriangle (startX, header.getCentreY() - 2.5f, startX + 7.0f, header.getCentreY() - 2.5f,
                              startX + 3.5f, header.getCentreY() + 2.5f);
        g.setColour (MugenColours::blue);
        g.fillPath (triangle);

        g.setColour (MugenColours::text);
        g.drawText (title, juce::Rectangle<float> (startX + 14.0f, header.getY(), textWidth + 4.0f, header.getHeight()),
                    juce::Justification::centredLeft);
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
        setColour (whiteNoteColourId,           juce::Colour (0xffdcd8f2));
        setColour (blackNoteColourId,           juce::Colour (0xff15132a));
        setColour (keySeparatorLineColourId,    juce::Colour (0xff3a3370));
        setColour (mouseOverKeyOverlayColourId, MugenColours::blue.withAlpha (0.25f));
        setColour (keyDownOverlayColourId,      MugenColours::violet.withAlpha (0.75f));
        setColour (shadowColourId,              juce::Colours::transparentBlack);
        setColour (upDownButtonBackgroundColourId, MugenColours::panel);
        setColour (upDownButtonArrowColourId,   MugenColours::blue);
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
        const float size = juce::jmin (area.getWidth() * 0.42f, 7.0f);
        const auto dot = juce::Rectangle<float> (size, size).withCentre ({ area.getCentreX(), area.getBottom() - size - 4.0f });

        g.setColour (isRoot ? MugenColours::blue : MugenColours::violet);
        g.fillEllipse (dot);
    }

    MugenSpaceDrumProcessor& processor;
};

// ==============================================================================
//  Capa da categoria (arte gerada, sem imagens externas)
// ==============================================================================
class ArtworkCard : public juce::Component
{
public:
    void setCategory (const juce::String& newCategory)
    {
        if (newCategory != category)
        {
            category = newCategory;
            repaint();
        }
    }

    void paint (juce::Graphics& g) override
    {
        const auto area = getLocalBounds().toFloat();
        const int seed = category.hashCode();
        juce::Random random (seed);

        // Cores por categoria
        juce::Colour top (0xff2b1d6e), bottom (0xff0a0818), glow (MugenColours::violet);
        if (category == "Organic")    { top = juce::Colour (0xff1d3a4a); glow = juce::Colour (0xff6fd3c1); }
        if (category == "Rhythmic")   { top = juce::Colour (0xff4a1d4a); glow = juce::Colour (0xffff7ab8); }
        if (category == "Atmosphere") { top = juce::Colour (0xff142a5e); glow = MugenColours::blue; }
        if (category == "Crystal")    { top = juce::Colour (0xff1d4a66); glow = juce::Colour (0xffbfefff); }
        if (category == "User")       { top = juce::Colour (0xff3a2a14); glow = juce::Colour (0xffffc46b); }

        juce::Path clip;
        clip.addRoundedRectangle (area, 10.0f);
        g.saveState();
        g.reduceClipRegion (clip);

        g.setGradientFill (juce::ColourGradient (top, 0.0f, 0.0f, bottom, 0.0f, area.getBottom(), false));
        g.fillRect (area);

        // Estrelas
        for (int i = 0; i < 60; ++i)
        {
            g.setColour (juce::Colours::white.withAlpha (0.15f + random.nextFloat() * 0.5f));
            const float s = random.nextFloat() * 1.6f + 0.6f;
            g.fillEllipse (random.nextFloat() * area.getWidth(), random.nextFloat() * area.getHeight(), s, s);
        }

        // Planeta com anel de luz
        const juce::Point<float> planet (area.getRight() - 52.0f, area.getY() + 44.0f);
        g.setGradientFill (juce::ColourGradient (glow.withAlpha (0.55f), planet.x, planet.y,
                                                 glow.withAlpha (0.0f), planet.x + 70.0f, planet.y, true));
        g.fillEllipse (juce::Rectangle<float> (140.0f, 140.0f).withCentre (planet));
        g.setGradientFill (juce::ColourGradient (glow.brighter (0.3f), planet.x - 14.0f, planet.y - 14.0f,
                                                 top.darker (0.6f), planet.x + 20.0f, planet.y + 24.0f, true));
        g.fillEllipse (juce::Rectangle<float> (48.0f, 48.0f).withCentre (planet));
        g.setColour (glow.withAlpha (0.8f));
        g.drawEllipse (juce::Rectangle<float> (92.0f, 22.0f).withCentre (planet), 1.4f);

        // Horizonte
        juce::Path hill;
        hill.startNewSubPath (0.0f, area.getBottom());
        hill.cubicTo (area.getWidth() * 0.3f, area.getBottom() - 34.0f, area.getWidth() * 0.6f, area.getBottom() - 10.0f,
                      area.getWidth(), area.getBottom() - 30.0f);
        hill.lineTo (area.getWidth(), area.getBottom());
        hill.closeSubPath();
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.fillPath (hill);

        // Nome da categoria
        g.setColour (juce::Colours::white);
        g.setFont (font (22.0f, true, 0.12f));
        g.drawText (category.toUpperCase(), area.reduced (14.0f).withTrimmedTop (area.getHeight() * 0.35f),
                    juce::Justification::bottomLeft);
        g.setColour (juce::Colours::white.withAlpha (0.6f));
        g.setFont (font (10.0f, false, 0.3f));
        g.drawText ("MUGEN SPACE DRUM", area.reduced (14.0f), juce::Justification::topLeft);

        g.restoreState();
        g.setColour (MugenColours::panelOutline);
        g.drawRoundedRectangle (area.reduced (0.5f), 10.0f, 1.0f);
    }

private:
    juce::String category { "All" };
};

// ==============================================================================
//  Browser de patches (barra lateral)
// ==============================================================================
class PatchBrowser : public juce::Component,
                     private juce::ListBoxModel
{
public:
    explicit PatchBrowser (PresetManager& manager)
        : presets (manager),
          previousButton ("previous", 0.5f, MugenColours::blue),
          nextButton ("next", 0.0f, MugenColours::blue)
    {
        categoryBox.addItem ("All Patches", 1);
        const auto categories = PresetManager::categoryOrder();
        for (int i = 0; i < categories.size(); ++i)
            categoryBox.addItem (categories[i], i + 2);
        categoryBox.setSelectedId (1, juce::dontSendNotification);
        categoryBox.onChange = [this] { rebuild(); };
        addAndMakeVisible (categoryBox);

        searchBox.setTextToShowWhenEmpty ("Search patches...", MugenColours::textDim);
        searchBox.setFont (font (13.0f));
        searchBox.setIndents (10, 7);
        searchBox.onTextChange = [this] { rebuild(); };
        addAndMakeVisible (searchBox);

        addAndMakeVisible (artwork);

        list.setModel (this);
        list.setRowHeight (24);
        addAndMakeVisible (list);

        for (auto* b : { &randomButton, &saveButton })
        {
            b->getProperties().set ("ghost", true);
            addAndMakeVisible (*b);
        }
        randomButton.setButtonText ("RANDOM");
        saveButton.setButtonText ("SAVE");
        addAndMakeVisible (previousButton);
        addAndMakeVisible (nextButton);

        previousButton.onClick = [this] { presets.loadPrevious(); notifyChosen(); };
        nextButton.onClick     = [this] { presets.loadNext();     notifyChosen(); };
        randomButton.onClick   = [this]
        {
            const int count = (int) presets.getEntries().size();
            if (count > 0)
                presets.load (juce::Random::getSystemRandom().nextInt (count));
            notifyChosen();
        };
        saveButton.onClick = [this] { if (onSaveRequested) onSaveRequested(); };

        rebuild();
    }

    std::function<void()> onPresetChosen, onSaveRequested;

    void rebuild()
    {
        rows.clear();
        const auto& entries = presets.getEntries();
        const int selected = categoryBox.getSelectedId();
        const auto categories = PresetManager::categoryOrder();
        const juce::String onlyCategory = selected > 1 ? categories[selected - 2] : juce::String();
        const auto search = searchBox.getText().trim();

        artwork.setCategory (onlyCategory.isEmpty() ? juce::String ("All") : onlyCategory);

        for (const auto& category : categories)
        {
            if (onlyCategory.isNotEmpty() && category != onlyCategory)
                continue;

            bool headerAdded = false;
            for (size_t i = 0; i < entries.size(); ++i)
            {
                if (entries[i].category != category)
                    continue;
                if (search.isNotEmpty() && ! entries[i].name.containsIgnoreCase (search))
                    continue;
                if (! headerAdded && onlyCategory.isEmpty())
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
        g.setColour (MugenColours::sidebar);
        g.fillAll();
        g.setColour (MugenColours::panelOutline);
        g.fillRect (getWidth() - 1, 0, 1, getHeight());

        // Título
        juce::Path triangle;
        triangle.addTriangle (16.0f, 18.0f, 24.0f, 18.0f, 20.0f, 23.0f);
        g.setColour (MugenColours::blue);
        g.fillPath (triangle);
        g.setColour (MugenColours::text);
        g.setFont (font (14.0f, true, 0.2f));
        g.drawText ("PATCHES", 32, 6, 200, 28, juce::Justification::centredLeft);

        // Fundo da lista
        g.setColour (juce::Colour (0xff0a0914));
        g.fillRoundedRectangle (list.getBounds().toFloat().expanded (4.0f), 8.0f);
        g.setColour (MugenColours::panelOutline);
        g.drawRoundedRectangle (list.getBounds().toFloat().expanded (4.0f), 8.0f, 1.0f);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (12, 0);
        area.removeFromTop (42);
        categoryBox.setBounds (area.removeFromTop (28));
        area.removeFromTop (8);
        searchBox.setBounds (area.removeFromTop (28));
        area.removeFromTop (10);
        artwork.setBounds (area.removeFromTop (128));
        area.removeFromTop (14);

        auto toolbar = area.removeFromBottom (44).withTrimmedBottom (12);
        area.removeFromBottom (10);
        list.setBounds (area.reduced (4, 0));

        previousButton.setBounds (toolbar.removeFromLeft (32).reduced (6));
        nextButton.setBounds (toolbar.removeFromLeft (32).reduced (6));
        toolbar.removeFromLeft (6);
        saveButton.setBounds (toolbar.removeFromRight (70));
        toolbar.removeFromRight (6);
        randomButton.setBounds (toolbar);
    }

private:
    struct Row
    {
        bool isHeader;
        juce::String text;
        int entryIndex;
    };

    void notifyChosen()
    {
        list.repaint();
        if (onPresetChosen)
            onPresetChosen();
    }

    int getNumRows() override { return (int) rows.size(); }

    void paintListBoxItem (int rowNumber, juce::Graphics& g, int width, int height, bool) override
    {
        if (rowNumber < 0 || rowNumber >= (int) rows.size())
            return;

        const auto& row = rows[(size_t) rowNumber];

        if (row.isHeader)
        {
            g.setColour (MugenColours::violet.interpolatedWith (MugenColours::blue, 0.4f));
            g.setFont (font (10.5f, true, 0.18f));
            g.drawText (row.text, 8, 0, width - 8, height - 2, juce::Justification::bottomLeft);
            return;
        }

        const bool isCurrent = presets.getCurrentIndex() == row.entryIndex;
        if (isCurrent)
        {
            g.setGradientFill (juce::ColourGradient (MugenColours::violet.withAlpha (0.55f), 0.0f, 0.0f,
                                                     MugenColours::blue.withAlpha (0.25f), (float) width, 0.0f, false));
            g.fillRoundedRectangle (1.0f, 1.0f, (float) width - 2.0f, (float) height - 2.0f, 5.0f);
        }

        g.setColour (isCurrent ? juce::Colours::white : MugenColours::text.withAlpha (0.82f));
        g.setFont (font (13.0f));
        g.drawText (row.text, 14, 0, width - 16, height, juce::Justification::centredLeft);
    }

    void listBoxItemClicked (int rowNumber, const juce::MouseEvent&) override
    {
        if (rowNumber < 0 || rowNumber >= (int) rows.size() || rows[(size_t) rowNumber].isHeader)
            return;

        presets.load (rows[(size_t) rowNumber].entryIndex);
        notifyChosen();
    }

    PresetManager& presets;
    juce::ComboBox categoryBox;
    juce::TextEditor searchBox;
    ArtworkCard artwork;
    juce::ListBox list;
    juce::ArrowButton previousButton, nextButton;
    juce::TextButton randomButton, saveButton;
    std::vector<Row> rows;
};

// ==============================================================================
//  Ecrã do preset no cabeçalho
// ==============================================================================
class PatchDisplay : public juce::Component
{
public:
    explicit PatchDisplay (PresetManager& manager)
        : presets (manager),
          upButton ("previous", 0.75f, MugenColours::blue),
          downButton ("next", 0.25f, MugenColours::blue)
    {
        addAndMakeVisible (upButton);
        addAndMakeVisible (downButton);
        upButton.onClick   = [this] { presets.loadPrevious(); if (onPresetChosen) onPresetChosen(); };
        downButton.onClick = [this] { presets.loadNext();     if (onPresetChosen) onPresetChosen(); };
    }

    std::function<void()> onPresetChosen;

    void setPreset (const juce::String& name, const juce::String& category)
    {
        presetName = name;
        presetCategory = category;
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        const auto area = getLocalBounds().toFloat();
        g.setColour (juce::Colour (0xe0080714));
        g.fillRoundedRectangle (area, 8.0f);
        g.setColour (isMouseOver() ? MugenColours::violet : juce::Colour (0xff3b3577));
        g.drawRoundedRectangle (area.reduced (0.5f), 8.0f, 1.0f);

        // Etiqueta da categoria
        g.setColour (MugenColours::violet.interpolatedWith (MugenColours::blue, 0.5f));
        g.setFont (font (9.5f, true, 0.2f));
        g.drawText (presetCategory.toUpperCase(), getLocalBounds().withWidth (90).withTrimmedLeft (12),
                    juce::Justification::centredLeft);

        g.setColour (MugenColours::text);
        g.setFont (font (18.0f));
        g.drawText (presetName, getLocalBounds().reduced (96, 0), juce::Justification::centred, true);
    }

    void resized() override
    {
        auto column = getLocalBounds().removeFromRight (30).reduced (6, 5);
        upButton.setBounds (column.removeFromTop (column.getHeight() / 2).reduced (2));
        downButton.setBounds (column.reduced (2));
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (e.x > getWidth() - 34)
            return;

        juce::PopupMenu menu;
        const auto& entries = presets.getEntries();
        const int current = presets.getCurrentIndex();

        for (const auto& category : PresetManager::categoryOrder())
        {
            juce::PopupMenu sub;
            for (size_t i = 0; i < entries.size(); ++i)
                if (entries[i].category == category)
                    sub.addItem ((int) i + 1, entries[i].name, true, (int) i == current);
            if (sub.getNumItems() > 0)
                menu.addSubMenu (category, sub);
        }

        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
                            [this] (int result)
                            {
                                if (result > 0)
                                {
                                    presets.load (result - 1);
                                    if (onPresetChosen)
                                        onPresetChosen();
                                }
                            });
    }

    void mouseEnter (const juce::MouseEvent&) override { repaint(); }
    void mouseExit (const juce::MouseEvent&) override  { repaint(); }

private:
    PresetManager& presets;
    juce::ArrowButton upButton, downButton;
    juce::String presetName, presetCategory;
};

// ==============================================================================
//  Visual: círculo da escala (12 notas, escala ligada por linhas)
// ==============================================================================
class ScaleCircle : public juce::Component
{
public:
    explicit ScaleCircle (MugenSpaceDrumProcessor& p) : processor (p) {}

    void paint (juce::Graphics& g) override
    {
        const int root  = juce::roundToInt (processor.parameters.getRawParameterValue (ParamIDs::scaleRoot)->load());
        const int scale = juce::roundToInt (processor.parameters.getRawParameterValue (ParamIDs::scaleType)->load());
        const auto names = ScaleEngine::rootNames();

        const auto area = getLocalBounds().toFloat();
        const auto centre = area.getCentre();
        const float radius = juce::jmin (area.getWidth(), area.getHeight()) * 0.36f;

        g.setColour (MugenColours::panelOutline);
        g.drawEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centre), 1.0f);

        juce::Path shape;
        bool first = true;
        for (int i = 0; i < 12; ++i)
        {
            const int note = (root + i) % 12;
            if (! ScaleEngine::isInScale (note, root, scale))
                continue;
            const auto p = centre.getPointOnCircumference (radius, juce::MathConstants<float>::twoPi * (float) i / 12.0f);
            if (first) { shape.startNewSubPath (p); first = false; }
            else       shape.lineTo (p);
        }
        shape.closeSubPath();
        g.setColour (MugenColours::violet.withAlpha (0.12f));
        g.fillPath (shape);
        g.setColour (MugenColours::violet.withAlpha (0.6f));
        g.strokePath (shape, juce::PathStrokeType (1.2f));

        for (int i = 0; i < 12; ++i)
        {
            const int note = (root + i) % 12;
            const float a = juce::MathConstants<float>::twoPi * (float) i / 12.0f;
            const auto dotPos = centre.getPointOnCircumference (radius, a);
            const auto labelPos = centre.getPointOnCircumference (radius + 18.0f, a);
            const bool inScale = ScaleEngine::isInScale (note, root, scale);
            const bool isRoot = i == 0;

            if (inScale)
            {
                const auto colour = isRoot ? MugenColours::blue : MugenColours::violet;
                g.setColour (colour.withAlpha (0.25f));
                g.fillEllipse (juce::Rectangle<float> (16.0f, 16.0f).withCentre (dotPos));
                g.setColour (colour);
                g.fillEllipse (juce::Rectangle<float> (8.0f, 8.0f).withCentre (dotPos));
            }
            else
            {
                g.setColour (MugenColours::knobTrack);
                g.fillEllipse (juce::Rectangle<float> (5.0f, 5.0f).withCentre (dotPos));
            }

            g.setColour (inScale ? MugenColours::text : MugenColours::textDim.withAlpha (0.5f));
            g.setFont (font (11.0f, isRoot));
            g.drawText (names[note], juce::Rectangle<float> (28.0f, 16.0f).withCentre (labelPos), juce::Justification::centred);
        }
    }

private:
    MugenSpaceDrumProcessor& processor;
};

// ==============================================================================
//  Visual: osciloscópio do movimento (esquerda e direita)
// ==============================================================================
class MotionScope : public juce::Component,
                    private juce::Timer
{
public:
    explicit MotionScope (MugenSpaceDrumProcessor& p) : processor (p) { startTimerHz (30); }
    ~MotionScope() override { stopTimer(); }

    void paint (juce::Graphics& g) override
    {
        const float depth = processor.parameters.getRawParameterValue (ParamIDs::motionDepth)->load();
        const float rate  = processor.parameters.getRawParameterValue (ParamIDs::motionRate)->load();
        const auto area = getLocalBounds().toFloat().reduced (6.0f);

        g.setColour (MugenColours::screen);
        g.fillRoundedRectangle (area, 8.0f);
        g.setColour (MugenColours::panelOutline);
        for (int i = 1; i < 4; ++i)
            g.fillRect (area.getX() + 8.0f, area.getY() + area.getHeight() * (float) i / 4.0f, area.getWidth() - 16.0f, 1.0f);

        for (int channel = 0; channel < 2; ++channel)
        {
            juce::Path wave;
            for (float x = 0.0f; x <= area.getWidth(); x += 2.0f)
            {
                const float u = x / area.getWidth();
                const float phase = juce::MathConstants<float>::twoPi * (u * 2.0f) + time * rate * 6.0f
                                  + (channel == 1 ? juce::MathConstants<float>::halfPi : 0.0f);
                const float y = area.getCentreY() + std::sin (phase) * area.getHeight() * 0.38f * (0.08f + depth * 0.92f);
                if (x == 0.0f) wave.startNewSubPath (area.getX() + x, y);
                else           wave.lineTo (area.getX() + x, y);
            }
            const auto colour = channel == 0 ? MugenColours::violet : MugenColours::blue;
            g.setColour (colour.withAlpha (0.25f));
            g.strokePath (wave, juce::PathStrokeType (5.0f));
            g.setColour (colour);
            g.strokePath (wave, juce::PathStrokeType (1.6f));
        }

        g.setColour (MugenColours::textDim);
        g.setFont (font (10.0f, true, 0.15f));
        g.drawText ("L", area.reduced (8.0f), juce::Justification::topLeft);
        g.setColour (MugenColours::blue);
        g.drawText ("R", area.reduced (8.0f).withTrimmedLeft (12.0f), juce::Justification::topLeft);
    }

private:
    void timerCallback() override
    {
        time += 1.0f / 30.0f;
        repaint();
    }

    MugenSpaceDrumProcessor& processor;
    float time = 0.0f;
};

// ==============================================================================
//  Painel de controlos (título, legenda, seletores, knobs ou um visual)
// ==============================================================================
class ControlPanel : public juce::Component
{
public:
    juce::String title, caption;
    std::vector<std::vector<juce::Component*>> toolRows;
    std::vector<std::pair<juce::Slider*, juce::Label*>> knobs;
    juce::Component* custom = nullptr;
    int knobsPerRow = 2;
    int maxKnobSize = 96;

    void paint (juce::Graphics& g) override
    {
        drawPanel (g, getLocalBounds(), title);
        if (caption.isNotEmpty())
        {
            g.setColour (MugenColours::textDim);
            g.setFont (font (11.0f));
            g.drawText (caption, getLocalBounds().withTrimmedTop (36).withHeight (16).reduced (8, 0), juce::Justification::centred);
        }
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (12, 10).withTrimmedTop (caption.isNotEmpty() ? 50 : 30);

        for (auto& row : toolRows)
        {
            auto line = area.removeFromTop (28);
            const int w = line.getWidth() / juce::jmax (1, (int) row.size());
            for (auto* c : row)
                c->setBounds (line.removeFromLeft (w).reduced (3, 0));
            area.removeFromTop (8);
        }

        if (custom != nullptr)
        {
            custom->setBounds (area);
            return;
        }

        const int count = (int) knobs.size();
        if (count == 0)
            return;

        const int perRow = juce::jmax (1, juce::jmin (knobsPerRow, count));
        const int rowsNeeded = (count + perRow - 1) / perRow;
        const int rowHeight = area.getHeight() / rowsNeeded;
        constexpr int valueHeight = 18, labelHeight = 18;

        for (int r = 0; r < rowsNeeded; ++r)
        {
            auto rowArea = area.removeFromTop (rowHeight);
            const int inThisRow = juce::jmin (perRow, count - r * perRow);
            const int cellWidth = rowArea.getWidth() / perRow;
            rowArea = rowArea.withSizeKeepingCentre (cellWidth * inThisRow, rowArea.getHeight());

            for (int k = 0; k < inThisRow; ++k)
            {
                auto [slider, label] = knobs[(size_t) (r * perRow + k)];
                auto cell = rowArea.removeFromLeft (cellWidth);

                // Knob, valor e nome agrupados e centrados na célula
                const int size = juce::jmin (maxKnobSize, cell.getWidth() - 8, cell.getHeight() - valueHeight - labelHeight - 6);
                auto group = cell.withSizeKeepingCentre (cell.getWidth(), size + valueHeight + labelHeight + 2);
                slider->setBounds (group.removeFromTop (size + valueHeight).withSizeKeepingCentre (juce::jmax (size, 84), size + valueHeight));
                label->setBounds (group.removeFromTop (labelHeight + 2));
            }
        }
    }
};

// ==============================================================================
//  MainView — tudo o que se vê, desenhado em 1280 x 800 e depois escalado
// ==============================================================================
class MugenSpaceDrumEditor::MainView : public juce::Component,
                                       private juce::Timer
{
public:
    explicit MainView (MugenSpaceDrumProcessor& p)
        : processor (p),
          browser (p.presetManager),
          patchDisplay (p.presetManager),
          orb (p),
          scaleCircle (p),
          motionScope (p),
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
        makeKnob (octave,      ParamIDs::octave,        "Octave");
        makeKnob (tune,        ParamIDs::fineTune,      "Tune");

        // Fader do volume (faixa OUTPUT)
        volumeFader.setSliderStyle (juce::Slider::LinearVertical);
        volumeFader.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 18);
        styleValueBox (volumeFader);
        volumeAttachment = std::make_unique<SliderAttachment> (processor.parameters, ParamIDs::volume, volumeFader);
        addAndMakeVisible (volumeFader);

        // Macros (sempre visíveis)
        makeMacro (macros[0], ParamIDs::character,   "CHARACTER");
        makeMacro (macros[1], ParamIDs::blend,       "BLEND");
        makeMacro (macros[2], ParamIDs::reverbMix,   "SPACE");
        makeMacro (macros[3], ParamIDs::motionDepth, "MOTION");

        // --- Seletores e botões ---------------------------------------------------------
        makeSelector (sampleSelector,   SampleBank::typeNames(),     ParamIDs::sampleType);
        makeSelector (divisionSelector, DelayDivisions::names(),     ParamIDs::delayDivision);
        makeSelector (reverbSelector,   ReverbEngine::typeNames(),   ParamIDs::reverbType);
        makeSelector (rootSelector,     ScaleEngine::rootNames(),    ParamIDs::scaleRoot);
        makeSelector (scaleSelector,    ScaleEngine::scaleNames(),   ParamIDs::scaleType);
        makeSelector (mappingSelector,  ScaleEngine::mappingNames(), ParamIDs::scaleMapping);
        makeToggle (syncButton,   "SYNC",   ParamIDs::delaySync);
        makeToggle (freezeButton, "FREEZE", ParamIDs::freeze);

        // --- Painéis ------------------------------------------------------------------
        setupPanels();

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

        browser.onPresetChosen = [this] { presetChanged(); };
        browser.onSaveRequested = [this] { savePreset(); };
        patchDisplay.onPresetChosen = [this] { presetChanged(); };
        addAndMakeVisible (browser);
        addAndMakeVisible (patchDisplay);
        addAndMakeVisible (keyboard);

        // Estrelas da faixa do cabeçalho (sempre iguais)
        juce::Random random (91);
        for (int i = 0; i < 90; ++i)
            stars.push_back ({ (float) headerBandArea.getX() + random.nextFloat() * (float) headerBandArea.getWidth(),
                               (float) headerBandArea.getY() + random.nextFloat() * (float) headerBandArea.getHeight() });

        const int savedTab = juce::jlimit (0, 4, (int) processor.parameters.state.getProperty ("uiTab", 0));
        tabs[(size_t) savedTab].setToggleState (true, juce::dontSendNotification);
        showPage (savedTab);

        presetChanged();
        startTimerHz (30);
    }

    ~MainView() override { stopTimer(); }

    void paint (juce::Graphics& g) override
    {
        // Corpo
        g.setColour (MugenColours::body);
        g.fillAll();

        // Barra de topo
        g.setColour (MugenColours::topBar);
        g.fillRect (topBarArea);
        g.setColour (MugenColours::panelOutline);
        g.fillRect (topBarArea.withTop (topBarArea.getBottom() - 1));

        g.setColour (MugenColours::text);
        g.setFont (font (12.0f, true, 0.3f));
        g.drawText ("MUGEN", 18, 0, 80, 40, juce::Justification::centredLeft);
        g.setColour (MugenColours::textDim);
        g.setFont (font (11.0f, false, 0.3f));
        g.drawText ("SPACE DRUM", 82, 0, 120, 40, juce::Justification::centredLeft);

        g.setFont (font (11.0f));
        g.drawText ("v" JucePlugin_VersionString, 1080, 0, 80, 40, juce::Justification::centredRight);
        g.drawText ("MIDI", ledBounds.getX() - 40, 0, 34, 40, juce::Justification::centredRight);
        g.setColour (MugenColours::knobTrack);
        g.fillEllipse (ledBounds.toFloat());
        if (ledLevel > 0.01f)
        {
            g.setColour (MugenColours::blue.withAlpha (ledLevel * 0.35f));
            g.fillEllipse (ledBounds.toFloat().expanded (5.0f));
            g.setColour (MugenColours::blue.withAlpha (ledLevel));
            g.fillEllipse (ledBounds.toFloat());
        }

        // Faixa do cabeçalho: céu violeta-azul com estrelas
        {
            const auto band = headerBandArea.toFloat();
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2a1c6b), band.getX(), band.getY(),
                                                     juce::Colour (0xff17327a), band.getRight(), band.getBottom(), false));
            g.fillRect (band);
            g.setGradientFill (juce::ColourGradient (MugenColours::violet.withAlpha (0.35f), band.getCentreX(), band.getY() + 30.0f,
                                                     MugenColours::violet.withAlpha (0.0f), band.getCentreX() + 360.0f, band.getY() + 30.0f, true));
            g.fillRect (band);

            for (size_t i = 0; i < stars.size(); ++i)
            {
                g.setColour (juce::Colours::white.withAlpha ((i % 4 == 0) ? 0.5f : 0.18f));
                const float s = (i % 7 == 0) ? 1.8f : 1.0f;
                g.fillEllipse (stars[i].x, stars[i].y, s, s);
            }

            g.setColour (juce::Colours::black.withAlpha (0.35f));
            g.fillRect (band.withTop (band.getBottom() - 2.0f));

            // Logótipo
            g.setColour (juce::Colours::white);
            g.setFont (font (42.0f, false, 0.55f));
            g.drawText ("MUGEN", headerBandArea.withHeight (68).withTrimmedTop (8), juce::Justification::centred);

            g.setColour (juce::Colours::white.withAlpha (0.75f));
            g.setFont (font (11.0f, false, 0.6f));
            g.drawText ("SPACE DRUM", 290, 118, 220, 20, juce::Justification::centredLeft);
            g.drawText ("MADE BY MUGEN", 1030, 118, 230, 20, juce::Justification::centredRight);
        }

        // Faixas laterais e ecrã central
        drawPanel (g, tuningStripArea, "TUNING");
        drawPanel (g, outputStripArea, "OUTPUT");
        drawPanel (g, screenArea, "MUGEN ORB", true);

        // Legenda do ecrã
        g.setColour (MugenColours::blue);
        g.setFont (font (15.0f));
        g.drawText (screenCaption, screenArea.withTrimmedTop (screenArea.getHeight() - 40), juce::Justification::centred);

        // Macros
        {
            const auto area = macroArea.toFloat();
            g.setColour (MugenColours::panel);
            g.fillRoundedRectangle (area, 10.0f);
            g.setColour (MugenColours::panelOutline);
            g.drawRoundedRectangle (area.reduced (0.5f), 10.0f, 1.0f);
        }
    }

    void resized() override
    {
        ledBounds = { 1238, 14, 12, 12 };

        browser.setBounds (sidebarArea);
        patchDisplay.setBounds (patchDisplayArea);

        auto tabRow = tabRowArea;
        const int tabWidth = tabRow.getWidth() / (int) tabs.size();
        for (auto& tab : tabs)
            tab.setBounds (tabRow.removeFromLeft (tabWidth).reduced (3, 0));

        for (auto& page : pages)
        {
            page.left.setBounds (leftPanelArea);
            page.right.setBounds (rightPanelArea);
        }

        orb.setBounds (screenArea.withTrimmedTop (30).withTrimmedBottom (40).reduced (4));

        // Faixa TUNING: dois knobs em coluna
        tuningPanel.setBounds (tuningStripArea);

        // Faixa OUTPUT: fader do volume + knob Width
        auto output = outputStripArea.reduced (8).withTrimmedTop (30);
        auto widthArea = output.removeFromBottom (126);
        output.removeFromBottom (6);
        volumeLabel.setBounds (output.removeFromBottom (18));
        volumeFader.setBounds (output.reduced (0, 4));
        width.slider.setBounds (widthArea.removeFromTop (100).withSizeKeepingCentre (84, 100));
        width.label.setBounds (widthArea.removeFromTop (20));

        // Macros
        auto macroRow = macroArea.reduced (18, 6);
        const int macroWidth = macroRow.getWidth() / (int) macros.size();
        for (auto& macro : macros)
        {
            auto cell = macroRow.removeFromLeft (macroWidth).reduced (14, 0);
            macro.label.setBounds (cell.removeFromBottom (16));
            macro.slider.setBounds (cell);
        }

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

    struct TabPanels
    {
        ControlPanel left, right;
    };

    static void styleValueBox (juce::Slider& slider)
    {
        // Valor limpo, sem moldura
        slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        slider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        slider.setColour (juce::Slider::textBoxTextColourId, MugenColours::textDim);
    }

    void makeKnob (Knob& knob, const char* id, const juce::String& name)
    {
        knob.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        knob.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 96, 18);
        knob.slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                         juce::MathConstants<float>::pi * 2.75f, true);
        styleValueBox (knob.slider);
        knob.label.setText (name, juce::dontSendNotification);
        knob.label.setJustificationType (juce::Justification::centred);
        knob.label.setFont (font (12.5f, true, 0.05f));
        knob.label.setColour (juce::Label::textColourId, MugenColours::text);
        knob.attachment = std::make_unique<SliderAttachment> (processor.parameters, id, knob.slider);
    }

    struct Macro
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<SliderAttachment> attachment;
    };

    void makeMacro (Macro& macro, const char* id, const juce::String& name)
    {
        macro.slider.setSliderStyle (juce::Slider::LinearHorizontal);
        macro.slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        macro.label.setText (name, juce::dontSendNotification);
        macro.label.setJustificationType (juce::Justification::centred);
        macro.label.setFont (font (10.5f, true, 0.2f));
        macro.label.setColour (juce::Label::textColourId, MugenColours::textDim);
        macro.attachment = std::make_unique<SliderAttachment> (processor.parameters, id, macro.slider);
        addAndMakeVisible (macro.slider);
        addAndMakeVisible (macro.label);
    }

    void makeSelector (Selector& s, const juce::StringArray& items, const char* id)
    {
        s.box.addItemList (items, 1);
        s.attachment = std::make_unique<ComboBoxAttachment> (processor.parameters, id, s.box);
    }

    void makeToggle (Toggle& t, const juce::String& text, const char* id)
    {
        t.button.setButtonText (text);
        t.button.setClickingTogglesState (true);
        t.attachment = std::make_unique<ButtonAttachment> (processor.parameters, id, t.button);
    }

    void fillPanel (ControlPanel& panel, const juce::String& title, const juce::String& caption,
                    std::vector<std::vector<juce::Component*>> tools, std::vector<Knob*> knobList,
                    juce::Component* custom = nullptr)
    {
        panel.title = title;
        panel.caption = caption;
        panel.toolRows = tools;
        panel.custom = custom;
        if (knobList.size() == 1)
        {
            panel.knobsPerRow = 1;
            panel.maxKnobSize = 150;
        }

        for (auto& row : tools)
            for (auto* c : row)
                panel.addAndMakeVisible (c);
        for (auto* k : knobList)
        {
            panel.addAndMakeVisible (k->slider);
            panel.addAndMakeVisible (k->label);
            panel.knobs.push_back ({ &k->slider, &k->label });
        }
        if (custom != nullptr)
            panel.addAndMakeVisible (custom);
    }

    void setupPanels()
    {
        auto& instrument = pages[0];
        fillPanel (instrument.left, "BODY & MALLET", "Body shape and mallet hardness", {}, { &character, &brightness });
        fillPanel (instrument.right, "ORGANIC SAMPLE", "Generated inside the plugin", { { &sampleSelector.box } }, { &blend });

        auto& resonancePage = pages[1];
        fillPanel (resonancePage.left, "RESONANCE", "Ethereal beating of harmonics", {}, { &resonance });
        fillPanel (resonancePage.right, "SUSTAIN", "How long each note rings", {}, { &decay });

        auto& ambience = pages[2];
        fillPanel (ambience.left, "DELAY", "Warm ping-pong echoes",
                   { { &syncButton.button, &divisionSelector.box } }, { &delayMix, &delayTime, &feedback });
        fillPanel (ambience.right, "REVERB", "Room to Deep Space",
                   { { &reverbSelector.box, &freezeButton.button } }, { &reverbMix, &reverbSize, &shimmer });

        auto& motion = pages[3];
        fillPanel (motion.left, "MOVEMENT", "Slow hypnotic motion", {}, { &depth, &rate });
        fillPanel (motion.right, "STEREO FIELD", "Left and right movement", {}, {}, &motionScope);

        auto& scalePage = pages[4];
        fillPanel (scalePage.left, "SCALE", "Every key plays the scale",
                   { { &rootSelector.box }, { &scaleSelector.box }, { &mappingSelector.box } }, {});
        fillPanel (scalePage.right, "SCALE MAP", "Notes of the chosen scale", {}, {}, &scaleCircle);

        for (auto& page : pages)
        {
            addChildComponent (page.left);
            addChildComponent (page.right);
        }

        // Faixas laterais
        tuningPanel.title = "TUNING";
        tuningPanel.knobsPerRow = 1;
        tuningPanel.maxKnobSize = 64;
        for (auto* k : { &octave, &tune })
        {
            tuningPanel.addAndMakeVisible (k->slider);
            tuningPanel.addAndMakeVisible (k->label);
            tuningPanel.knobs.push_back ({ &k->slider, &k->label });
        }
        addAndMakeVisible (tuningPanel);

        addAndMakeVisible (width.slider);
        addAndMakeVisible (width.label);
        volumeLabel.setText ("Volume", juce::dontSendNotification);
        volumeLabel.setJustificationType (juce::Justification::centred);
        volumeLabel.setFont (font (12.5f, true, 0.05f));
        addAndMakeVisible (volumeLabel);
    }

    void showPage (int index)
    {
        currentPage = index;
        for (size_t i = 0; i < pages.size(); ++i)
        {
            pages[i].left.setVisible ((int) i == index);
            pages[i].right.setVisible ((int) i == index);
        }
        processor.parameters.state.setProperty ("uiTab", index, nullptr);
        updateScreenCaption();
    }

    juce::String valueText (const char* id) const
    {
        if (auto* param = processor.parameters.getParameter (id))
            return param->getCurrentValueAsText();
        return {};
    }

    void updateScreenCaption()
    {
        const auto dot = juce::String::fromUTF8 ("  \xc2\xb7  ");
        juce::String caption;
        switch (currentPage)
        {
            case 0:  caption = valueText (ParamIDs::sampleType) + dot + valueText (ParamIDs::character); break;
            case 1:  caption = "Resonance " + valueText (ParamIDs::resonance); break;
            case 2:  caption = valueText (ParamIDs::reverbType) + (processor.parameters.getRawParameterValue (ParamIDs::freeze)->load() > 0.5f ? dot + "Frozen" : juce::String()); break;
            case 3:  caption = "Motion " + valueText (ParamIDs::motionRate); break;
            default: caption = valueText (ParamIDs::scaleRoot) + " " + valueText (ParamIDs::scaleType); break;
        }

        if (caption != screenCaption)
        {
            screenCaption = caption;
            repaint (screenArea);
        }
    }

    void presetChanged()
    {
        lastPresetName = processor.presetManager.getCurrentName();
        const int index = processor.presetManager.getCurrentIndex();
        const auto& entries = processor.presetManager.getEntries();
        const auto category = (index >= 0 && index < (int) entries.size()) ? entries[(size_t) index].category : juce::String ("Custom");

        patchDisplay.setPreset (lastPresetName, category);
        browser.refreshHighlight();
        keyboard.repaint();
        scaleCircle.repaint();
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
        divisionSelector.box.setEnabled (synced);

        if (processor.presetManager.getCurrentName() != lastPresetName)
            presetChanged();

        const int signature = rootSelector.box.getSelectedItemIndex() * 1000 + scaleSelector.box.getSelectedItemIndex();
        if (signature != lastScaleSignature)
        {
            lastScaleSignature = signature;
            keyboard.repaint();
            scaleCircle.repaint();
        }

        updateScreenCaption();
    }

    MugenSpaceDrumProcessor& processor;

    Knob character, brightness, blend, resonance, decay,
         delayMix, delayTime, feedback, reverbMix, reverbSize, shimmer,
         depth, rate, width, octave, tune;

    juce::Slider volumeFader;
    juce::Label volumeLabel;
    std::unique_ptr<SliderAttachment> volumeAttachment;

    std::array<Macro, 4> macros;

    Selector sampleSelector, divisionSelector, reverbSelector, rootSelector, scaleSelector, mappingSelector;
    Toggle syncButton, freezeButton;

    PatchBrowser browser;
    PatchDisplay patchDisplay;
    OrbVisualizer orb;
    ScaleCircle scaleCircle;
    MotionScope motionScope;
    ScaleKeyboard keyboard;

    std::array<TabPanels, 5> pages;
    ControlPanel tuningPanel;
    std::array<juce::TextButton, 5> tabs;

    std::unique_ptr<juce::FileChooser> chooser;

    juce::Rectangle<int> ledBounds;
    float ledLevel = 0.0f;
    int currentPage = 0;
    juce::String lastPresetName, screenCaption;
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

    // Janela redimensionável, sempre com as mesmas proporções (abre a 85 %).
    setResizable (true, true);
    setResizeLimits (designWidth * 6 / 10, designHeight * 6 / 10, designWidth * 13 / 10, designHeight * 13 / 10);
    if (auto* sizeLimits = getConstrainer())
        sizeLimits->setFixedAspectRatio ((double) designWidth / (double) designHeight);

    setSize (designWidth * 85 / 100, designHeight * 85 / 100);
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
