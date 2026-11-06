#include "PluginEditor.h"
#include "MugenArt.h"

namespace
{
    using SliderAttachment   = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment   = juce::AudioProcessorValueTreeState::ButtonAttachment;

    // --- Mapa da janela (coordenadas de desenho, 1280 x 800) ----------------------
    const juce::Rectangle<int> topBarArea      { 0,    0,   1280, 40 };
    const juce::Rectangle<int> sidebarArea     { 0,    40,  264,  760 };
    const juce::Rectangle<int> headerBandArea  { 264,  40,  1016, 110 };
    const juce::Rectangle<int> patchDisplayArea{ 562,  100, 420,  38 };
    const juce::Rectangle<int> tabRowArea      { 276,  158, 992,  32 };
    const juce::Rectangle<int> leftPanelArea   { 276,  198, 220,  438 };
    const juce::Rectangle<int> stageArea       { 508,  198, 436,  438 };
    const juce::Rectangle<int> rightPanelArea  { 956,  198, 220,  438 };
    const juce::Rectangle<int> outputStripArea { 1188, 198, 80,   438 };
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

        // Corpo com gradiente vertical suave
        g.setGradientFill (juce::ColourGradient (screen ? MugenColours::screen : MugenColours::panelTop, 0.0f, area.getY(),
                                                 screen ? MugenColours::screen : MugenColours::panelBottom, 0.0f, area.getBottom(), false));
        g.fillRoundedRectangle (area, 12.0f);

        auto header = area.withHeight (30.0f);
        juce::Path headerShape;
        headerShape.addRoundedRectangle (header.getX(), header.getY(), header.getWidth(), header.getHeight(),
                                         12.0f, 12.0f, true, true, false, false);
        g.setGradientFill (juce::ColourGradient (MugenColours::headerHighlight, 0.0f, header.getY(),
                                                 MugenColours::panelHeader, 0.0f, header.getBottom(), false));
        g.fillPath (headerShape);

        // Contorno e fio de luz no topo
        g.setColour (MugenColours::panelOutline);
        g.drawRoundedRectangle (area.reduced (0.5f), 12.0f, 1.0f);
        g.fillRect (header.withTop (header.getBottom() - 1.0f).reduced (1.0f, 0.0f));

        juce::ColourGradient topLight (MugenColours::violet.withAlpha (0.0f), area.getX() + 12.0f, 0.0f,
                                       MugenColours::blue.withAlpha (0.0f), area.getRight() - 12.0f, 0.0f, false);
        topLight.addColour (0.5, MugenColours::violet.withAlpha (0.65f));
        g.setGradientFill (topLight);
        g.fillRect (juce::Rectangle<float> (area.getX() + 12.0f, area.getY(), area.getWidth() - 24.0f, 1.0f));

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
        refreshColours();
        setWantsKeyboardFocus (false);
    }

    void refreshColours()
    {
        setColour (whiteNoteColourId,           MugenColours::keyWhite);
        setColour (blackNoteColourId,           MugenColours::keyBlack);
        setColour (keySeparatorLineColourId,    MugenColours::keyLine);
        setColour (mouseOverKeyOverlayColourId, MugenColours::blue.withAlpha (0.25f));
        setColour (keyDownOverlayColourId,      MugenColours::violet.withAlpha (0.75f));
        setColour (shadowColourId,              juce::Colours::transparentBlack);
        setColour (upDownButtonBackgroundColourId, MugenColours::panel);
        setColour (upDownButtonArrowColourId,   MugenColours::blue);
        repaint();
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
        juce::Colour top (MugenColours::bandA), bottom (MugenColours::screen), glow (MugenColours::violet);
        if (category == "Organic")    { top = juce::Colour (0xff1d3a4a); glow = juce::Colour (0xff6fd3c1); }
        if (category == "Rhythmic")   { top = juce::Colour (0xff4a1d4a); glow = juce::Colour (0xffff7ab8); }
        if (category == "Atmosphere") { top = juce::Colour (0xff142a5e); glow = MugenColours::blue; }
        if (category == "Crystal")    { top = juce::Colour (0xff1d4a66); glow = juce::Colour (0xffbfefff); }
        if (category == "Sequences")  { top = juce::Colour (0xff3a1d5e); glow = juce::Colour (0xffd59bff); }
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

    void refreshColours()
    {
        searchBox.setTextToShowWhenEmpty ("Search patches...", MugenColours::textDim);
        searchBox.applyColourToAllText (MugenColours::text);
        artwork.repaint();
        list.repaint();
    }

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
        g.setColour (MugenColours::listBackground);
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
        g.setColour (MugenColours::screen.withAlpha (0.88f));
        g.fillRoundedRectangle (area, 8.0f);
        g.setColour (isMouseOver() ? MugenColours::violet : MugenColours::displayEdge);
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
//  Logótipo MUGEN 3D (animado: brilho que atravessa as letras e cintilações)
// ==============================================================================
class MugenLogo : public juce::Component,
                  private juce::Timer
{
public:
    MugenLogo()
    {
        setInterceptsMouseClicks (false, false);
        startTimerHz (30);
    }

    ~MugenLogo() override { stopTimer(); }

    void paint (juce::Graphics& g) override
    {
        const MugenArt::Palette palette { MugenColours::violet, MugenColours::blue, MugenColours::text, MugenColours::screen };
        MugenArt::drawLogo3D (g, getLocalBounds().toFloat().reduced (8.0f, 0.0f), "MUGEN", palette, time);
    }

private:
    void timerCallback() override
    {
        time += 1.0f / 30.0f;
        repaint();
    }

    float time = 0.0f;
};

// ==============================================================================
//  Estrelas cadentes — passam pelo fundo e atravessam os painéis em transparência
// ==============================================================================
class ShootingStars : public juce::Component,
                      private juce::Timer
{
public:
    struct Glass
    {
        juce::Rectangle<int> area;
        float visibility;   // 1 = fundo (brilho total), 0 = não aparece
    };

    ShootingStars()
    {
        setInterceptsMouseClicks (false, false);
        startTimerHz (60);
    }

    ~ShootingStars() override { stopTimer(); }

    void setGlassAreas (std::vector<Glass> areas) { glass = std::move (areas); }

    void setActive (bool shouldShow)
    {
        active = shouldShow;
        if (! active)
        {
            for (auto& m : meteors)
                m.alive = false;
            repaint();
        }
    }

    void paint (juce::Graphics& g) override
    {
        for (const auto& m : meteors)
        {
            if (! m.alive)
                continue;

            // 1) Brilho total por cima do fundo (fora dos painéis)
            g.saveState();
            for (const auto& gl : glass)
                g.excludeClipRegion (gl.area);
            drawMeteor (g, m, 1.0f);
            g.restoreState();

            // 2) Dentro dos painéis: mais suave, como se passasse por trás de vidro
            for (const auto& gl : glass)
            {
                if (gl.visibility <= 0.0f || ! gl.area.toFloat().intersects (m.bounds()))
                    continue;
                g.saveState();
                g.reduceClipRegion (gl.area);
                drawMeteor (g, m, gl.visibility);
                g.restoreState();
            }
        }
    }

private:
    struct Meteor
    {
        bool alive = false;
        juce::Point<float> head, direction;
        float speed = 700.0f, length = 200.0f, life = 0.0f, duration = 1.2f, thickness = 1.6f;
        juce::Colour colour;

        juce::Point<float> tail() const
        {
            const float envelope = std::sin (juce::MathConstants<float>::pi * juce::jlimit (0.0f, 1.0f, life / duration));
            return head - direction * (length * (0.35f + 0.65f * envelope));
        }

        juce::Rectangle<float> bounds() const
        {
            return juce::Rectangle<float> (head, tail()).expanded (14.0f);
        }
    };

    static void drawMeteor (juce::Graphics& g, const Meteor& m, float visibility)
    {
        const float envelope = std::sin (juce::MathConstants<float>::pi * juce::jlimit (0.0f, 1.0f, m.life / m.duration));
        const float alpha = envelope * visibility;
        if (alpha <= 0.01f)
            return;

        const auto tail = m.tail();
        const juce::Line<float> line (tail, m.head);

        // Brilho largo
        g.setGradientFill (juce::ColourGradient (m.colour.withAlpha (0.0f), tail, m.colour.withAlpha (0.28f * alpha), m.head, false));
        g.drawLine (line, m.thickness * 5.0f);

        // Rasto fino (o núcleo puxa para branco, as pontas mantêm a cor)
        const auto core = m.colour.interpolatedWith (juce::Colours::white, 0.55f);
        g.setGradientFill (juce::ColourGradient (m.colour.withAlpha (0.0f), tail, core.withAlpha (0.95f * alpha), m.head, false));
        g.drawLine (line, m.thickness);

        // Cabeça luminosa
        g.setColour (m.colour.withAlpha (0.35f * alpha));
        g.fillEllipse (juce::Rectangle<float> (m.thickness * 7.0f, m.thickness * 7.0f).withCentre (m.head));
        g.setColour (juce::Colours::white.withAlpha (alpha));
        g.fillEllipse (juce::Rectangle<float> (m.thickness * 2.2f, m.thickness * 2.2f).withCentre (m.head));
    }

    void spawn()
    {
        for (auto& m : meteors)
        {
            if (m.alive)
                continue;

            const bool goingRight = random.nextFloat() < 0.65f;
            const float angle = juce::degreesToRadians (18.0f + random.nextFloat() * 22.0f);
            m.direction = { (goingRight ? 1.0f : -1.0f) * std::cos (angle), std::sin (angle) };
            m.head = { random.nextFloat() * (float) getWidth(), random.nextFloat() * (float) getHeight() * 0.55f };
            m.speed = 650.0f + random.nextFloat() * 450.0f;
            m.length = 140.0f + random.nextFloat() * 150.0f;
            m.duration = 0.9f + random.nextFloat() * 0.7f;
            m.life = 0.0f;

            const bool fireball = random.nextFloat() < 0.10f;
            m.thickness = fireball ? 2.6f : 1.0f + random.nextFloat() * 0.8f;
            if (fireball)
                m.length *= 1.4f;

            // Paleta: muitas brancas, e cores que vivem no plugin
            const float pick = random.nextFloat();
            if      (pick < 0.40f) m.colour = juce::Colour (0xfff4f1ff);   // branca
            else if (pick < 0.58f) m.colour = MugenColours::blue;          // acento secundário do tema
            else if (pick < 0.74f) m.colour = MugenColours::violet;        // acento principal do tema
            else if (pick < 0.85f) m.colour = MugenColours::starA;         // cores próprias de cada tema
            else if (pick < 0.94f) m.colour = MugenColours::starB;
            else                   m.colour = MugenColours::starC;         // a mais rara
            m.alive = true;
            return;
        }
    }

    void timerCallback() override
    {
        if (! active || ! isShowing())
            return;

        constexpr float dt = 1.0f / 60.0f;

        // Em média ~1,2 estrelas por segundo, e de vez em quando uma pequena chuva
        if (random.nextFloat() < dt * 1.2f)
            spawn();
        if (random.nextFloat() < dt / 14.0f)
            for (int i = 0; i < 4 + random.nextInt (4); ++i)
                spawn();

        for (auto& m : meteors)
        {
            if (! m.alive)
                continue;

            const auto before = m.bounds();
            m.head += m.direction * (m.speed * dt);
            m.life += dt;
            if (m.life >= m.duration)
                m.alive = false;

            repaint (before.getUnion (m.bounds()).getSmallestIntegerContainer());
        }
    }

    std::array<Meteor, 14> meteors;
    std::vector<Glass> glass;
    juce::Random random;
    bool active = true;
};

// ==============================================================================
//  Palco central: orbe + carrossel de presets animado + etiquetas do som
// ==============================================================================
class PresetStage : public juce::Component,
                    private juce::Timer
{
public:
    PresetStage (MugenSpaceDrumProcessor& p, juce::Component& centrePiece)
        : processor (p), orb (centrePiece)
    {
        addAndMakeVisible (orb);
        lastIndex = processor.presetManager.getCurrentIndex();
        startTimerHz (60);
    }

    ~PresetStage() override { stopTimer(); }

    std::function<void()> onPresetChosen;

    void paint (juce::Graphics& g) override
    {
        const auto area = getLocalBounds().toFloat();

        // Ecrã: fundo profundo com nebulosa
        g.setColour (MugenColours::screen);
        g.fillRoundedRectangle (area, 14.0f);
        g.setGradientFill (juce::ColourGradient (MugenColours::violet.withAlpha (0.20f), area.getCentreX(), area.getY() + 150.0f,
                                                 MugenColours::violet.withAlpha (0.0f), area.getCentreX() + 260.0f, area.getY() + 150.0f, true));
        g.fillRoundedRectangle (area, 14.0f);

        // Grelha subtil (look de ecrã técnico)
        g.setColour (juce::Colours::white.withAlpha (0.025f));
        for (float x = area.getX() + 18.0f; x < area.getRight(); x += 24.0f)
            g.fillRect (x, area.getY() + 40.0f, 1.0f, area.getHeight() - 150.0f);
        for (float y = area.getY() + 40.0f; y < area.getBottom() - 110.0f; y += 24.0f)
            g.fillRect (area.getX() + 8.0f, y, area.getWidth() - 16.0f, 1.0f);

        g.setColour (MugenColours::panelOutline);
        g.drawRoundedRectangle (area.reduced (0.5f), 14.0f, 1.0f);

        drawStatusBar (g);
        drawCarousel (g);
        drawChips (g);
    }

    void resized() override
    {
        orb.setBounds (getLocalBounds().withTrimmedTop (34).withTrimmedBottom (132));
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (! carouselArea().contains (e.getPosition()))
            return;

        const float dx = (float) e.x - (float) getWidth() * 0.5f;
        if (dx < -70.0f)       step (-1);
        else if (dx > 70.0f)   step (1);
    }

    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& wheel) override
    {
        wheelAccumulator += wheel.deltaY + wheel.deltaX;
        if (std::abs (wheelAccumulator) > 0.25f)
        {
            step (wheelAccumulator > 0.0f ? -1 : 1);
            wheelAccumulator = 0.0f;
        }
    }

private:
    juce::Rectangle<int> carouselArea() const { return { 0, getHeight() - 128, getWidth(), 70 }; }

    void step (int direction)
    {
        if (direction < 0) processor.presetManager.loadPrevious();
        else               processor.presetManager.loadNext();
        if (onPresetChosen)
            onPresetChosen();
    }

    void timerCallback() override
    {
        const int index = processor.presetManager.getCurrentIndex();
        const int count = (int) processor.presetManager.getEntries().size();

        if (index != lastIndex)
        {
            // Direção mais curta (o carrossel dá a volta)
            int delta = index - lastIndex;
            if (count > 0 && lastIndex >= 0 && index >= 0)
            {
                if (delta >  count / 2) delta -= count;
                if (delta < -count / 2) delta += count;
            }
            offset += (float) juce::jlimit (-3, 3, delta);
            flash = 1.0f;
            lastIndex = index;
        }

        offset *= 0.84f;
        if (std::abs (offset) < 0.001f)
            offset = 0.0f;
        flash *= 0.95f;
        time += 1.0f / 60.0f;

        repaint (getLocalBounds().withTrimmedTop (getHeight() - 132));
        repaint (getLocalBounds().withHeight (34));
    }

    void drawStatusBar (juce::Graphics& g)
    {
        const auto& entries = processor.presetManager.getEntries();
        const int index = processor.presetManager.getCurrentIndex();
        const auto bar = getLocalBounds().withHeight (34).reduced (14, 0);

        // Indicador "ao vivo"
        const float pulse = 0.5f + 0.5f * std::sin (time * 3.0f);
        g.setColour (MugenColours::blue.withAlpha (0.35f + 0.5f * pulse));
        g.fillEllipse ((float) bar.getX(), (float) bar.getCentreY() - 3.5f, 7.0f, 7.0f);
        g.setColour (MugenColours::textDim);
        g.setFont (font (10.5f, true, 0.25f));
        g.drawText ("LIVE ENGINE", bar.withTrimmedLeft (14), juce::Justification::centredLeft);

        const juce::String counter = (index >= 0 ? juce::String (index + 1).paddedLeft ('0', 2) : juce::String ("--"))
                                   + " / " + juce::String ((int) entries.size()).paddedLeft ('0', 2);
        g.drawText ("PRESET " + counter, bar, juce::Justification::centredRight);

        // Ghost Notes e expressão ao centro
        const bool ghostOn = processor.parameters.getRawParameterValue (ParamIDs::ghostOn)->load() > 0.5f;
        const bool ghostPlaying = processor.ghostActive.load();
        const float cx = (float) bar.getCentreX();
        const auto pill = juce::Rectangle<float> (cx - 84.0f, (float) bar.getCentreY() - 9.0f, 56.0f, 18.0f);
        if (ghostPlaying)
        {
            g.setColour (MugenColours::violet.withAlpha (0.25f + 0.25f * pulse));
            g.fillRoundedRectangle (pill.expanded (3.0f), 11.0f);
        }
        g.setColour (ghostOn ? MugenColours::violet : MugenColours::knobTrack);
        g.drawRoundedRectangle (pill, 9.0f, 1.0f);
        g.setColour (ghostOn ? juce::Colours::white : MugenColours::textDim.withAlpha (0.6f));
        g.setFont (font (9.5f, true, 0.25f));
        g.drawText ("GHOST", pill, juce::Justification::centred);

        const float expr = processor.expressionLevel.load();
        const auto label = juce::Rectangle<float> (cx - 18.0f, (float) bar.getY(), 34.0f, (float) bar.getHeight());
        const auto meter = juce::Rectangle<float> (cx + 20.0f, (float) bar.getCentreY() - 2.0f, 58.0f, 4.0f);
        g.setColour (MugenColours::textDim);
        g.drawText ("EXPR", label, juce::Justification::centredLeft);
        g.setColour (MugenColours::knobTrack);
        g.fillRoundedRectangle (meter, 2.0f);
        if (expr > 0.005f)
        {
            g.setGradientFill (juce::ColourGradient (MugenColours::violet, meter.getX(), 0.0f, MugenColours::blue, meter.getRight(), 0.0f, false));
            g.fillRoundedRectangle (meter.withWidth (meter.getWidth() * expr), 2.0f);
        }
    }

    void drawCarousel (juce::Graphics& g)
    {
        const auto& entries = processor.presetManager.getEntries();
        const int count = (int) entries.size();
        const int index = processor.presetManager.getCurrentIndex();
        const auto area = carouselArea().toFloat();
        const float centreX = area.getCentreX();
        const float spacing = 185.0f;

        g.saveState();
        g.reduceClipRegion (carouselArea().reduced (10, 0));

        // Linha de luz por baixo do nome atual
        juce::ColourGradient line (MugenColours::violet.withAlpha (0.0f), area.getX() + 30.0f, 0.0f,
                                   MugenColours::blue.withAlpha (0.0f), area.getRight() - 30.0f, 0.0f, false);
        line.addColour (0.5, MugenColours::blue.withAlpha (0.55f + 0.45f * flash));
        g.setGradientFill (line);
        g.fillRect (juce::Rectangle<float> (area.getX() + 30.0f, area.getBottom() - 8.0f, area.getWidth() - 60.0f, 1.0f));

        if (count > 0 && index >= 0)
        {
            for (int k = -3; k <= 3; ++k)
            {
                const int entryIndex = ((index + k) % count + count) % count;
                const float position = (float) k + offset;
                const float distance = std::abs (position);
                if (distance > 2.6f)
                    continue;

                const float x = centreX + position * spacing;
                const float closeness = juce::jlimit (0.0f, 1.0f, 1.0f - distance);
                const float alpha = juce::jlimit (0.0f, 1.0f, 1.0f - distance * 0.42f);
                const float size = 14.0f + 10.0f * closeness;

                const auto& entry = entries[(size_t) entryIndex];
                const float boxWidth = closeness > 0.5f ? 330.0f : 200.0f;
                const auto textArea = juce::Rectangle<float> (x - boxWidth * 0.5f, area.getY() + 10.0f, boxWidth, 34.0f);

                // Nomes longos encolhem para caber inteiros
                float fittedSize = size;
                while (fittedSize > 12.0f
                       && juce::GlyphArrangement::getStringWidth (font (fittedSize, closeness > 0.5f, 0.04f), entry.name) > boxWidth - 12.0f)
                    fittedSize -= 0.5f;

                if (closeness > 0.05f)
                {
                    g.setColour (MugenColours::violet.withAlpha (0.22f * closeness));
                    g.setFont (font (fittedSize, false, 0.04f));
                    g.drawText (entry.name, textArea.translated (0.0f, 1.5f).expanded (2.0f, 0.0f), juce::Justification::centred, false);
                }

                g.setColour ((closeness > 0.5f ? juce::Colours::white : MugenColours::textDim).withAlpha (alpha));
                g.setFont (font (fittedSize, closeness > 0.5f, 0.04f));
                g.drawText (entry.name, textArea, juce::Justification::centred, false);

                g.setColour (MugenColours::violet.interpolatedWith (MugenColours::blue, 0.5f).withAlpha (alpha * (0.4f + 0.6f * closeness)));
                g.setFont (font (9.5f, true, 0.25f));
                g.drawText (entry.category.toUpperCase(), textArea.translated (0.0f, 30.0f).withHeight (14.0f), juce::Justification::centred);
            }
        }

        g.restoreState();

        // As pontas do carrossel desvanecem no ecrã (os vizinhos "espreitam")
        for (int side : { -1, 1 })
        {
            const float edge = side < 0 ? area.getX() : area.getRight();
            const float inner = edge - (float) side * 86.0f;
            g.setGradientFill (juce::ColourGradient (MugenColours::screen, edge, 0.0f, MugenColours::screen.withAlpha (0.0f), inner, 0.0f, false));
            g.fillRect (juce::Rectangle<float> (juce::jmin (edge, inner), area.getY(), 86.0f, area.getHeight()));
        }

        // Setas laterais
        for (int side : { -1, 1 })
        {
            const float cx = side < 0 ? area.getX() + 18.0f : area.getRight() - 18.0f;
            juce::Path chevron;
            chevron.startNewSubPath (cx + 4.0f * (float) side * -1.0f, area.getCentreY() - 11.0f);
            chevron.lineTo (cx + 4.0f * (float) side, area.getCentreY() - 4.0f);
            chevron.lineTo (cx + 4.0f * (float) side * -1.0f, area.getCentreY() + 3.0f);
            g.setColour (MugenColours::blue.withAlpha (0.8f));
            g.strokePath (chevron, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
    }

    void drawChips (juce::Graphics& g)
    {
        auto text = [this] (const char* id)
        {
            if (auto* param = processor.parameters.getParameter (id))
                return param->getCurrentValueAsText();
            return juce::String();
        };

        const juce::StringArray chips {
            text (ParamIDs::sampleType),
            text (ParamIDs::character).upToFirstOccurrenceOf (" ", false, false),
            text (ParamIDs::reverbType),
            text (ParamIDs::scaleRoot) + " " + text (ParamIDs::scaleType)
        };

        const auto chipFont = font (11.0f, true, 0.08f);
        constexpr float gap = 8.0f, padding = 14.0f, height = 24.0f;

        float total = 0.0f;
        std::vector<float> widths;
        for (const auto& c : chips)
        {
            widths.push_back (juce::GlyphArrangement::getStringWidth (chipFont, c) + padding * 2.0f);
            total += widths.back();
        }
        total += gap * (float) (chips.size() - 1);

        // Se não couber tudo (escalas com nomes longos), tira a etiqueta do corpo
        if (total > (float) getWidth() - 24.0f)
        {
            total -= widths[1] + gap;
            widths[1] = -1.0f;
        }

        float x = (float) getWidth() * 0.5f - total * 0.5f;
        const float y = (float) getHeight() - 46.0f;

        g.setFont (chipFont);
        for (int i = 0; i < chips.size(); ++i)
        {
            if (widths[(size_t) i] < 0.0f)
                continue;
            const auto chip = juce::Rectangle<float> (x, y, widths[(size_t) i], height);
            g.setColour (MugenColours::chip);
            g.fillRoundedRectangle (chip, height * 0.5f);
            g.setColour ((i % 2 == 0 ? MugenColours::violet : MugenColours::blue).withAlpha (0.55f));
            g.drawRoundedRectangle (chip.reduced (0.5f), height * 0.5f, 1.0f);
            g.setColour (MugenColours::text);
            g.drawText (chips[i], chip, juce::Justification::centred);
            x += widths[(size_t) i] + gap;
        }
    }

    MugenSpaceDrumProcessor& processor;
    juce::Component& orb;

    int lastIndex = -1;
    float offset = 0.0f, flash = 0.0f, time = 0.0f, wheelAccumulator = 0.0f;
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
          keyboard (p.keyboardState, p),
          stage (p, orb)
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
        makeKnob (ghostDensity, ParamIDs::ghostDensity, "Density");
        makeKnob (ghostRange,   ParamIDs::ghostRange,   "Range");
        makeKnob (velocitySens, ParamIDs::velocitySens, "Velocity");
        makeKnob (wheelAmount,  ParamIDs::wheelAmount,  "Mod Wheel");
        makeKnob (pressureAmount, ParamIDs::pressureAmount, "Aftertouch");

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
        {
            // Menu das escalas com títulos por família
            const auto names = ScaleEngine::scaleNames();
            const auto& groups = ScaleEngine::scaleGroups();
            for (int i = 0; i < names.size(); ++i)
            {
                for (const auto& group : groups)
                    if (group.first == i)
                        scaleSelector.box.addSectionHeading (group.second);
                scaleSelector.box.addItem (names[i], i + 1);
            }
            scaleSelector.attachment = std::make_unique<ComboBoxAttachment> (processor.parameters, ParamIDs::scaleType, scaleSelector.box);
        }
        makeSelector (mappingSelector,  ScaleEngine::mappingNames(), ParamIDs::scaleMapping);
        makeToggle (syncButton,   "SYNC",   ParamIDs::delaySync);
        makeToggle (freezeButton, "FREEZE", ParamIDs::freeze);
        makeToggle (ghostButton,  "GHOST ON", ParamIDs::ghostOn);
        makeSelector (ghostRateSelector, GhostEngine::rateNames(), ParamIDs::ghostRate);
        makeSelector (ghostModeSelector, GhostEngine::modeNames(), ParamIDs::ghostMode);

        // --- Painéis ------------------------------------------------------------------
        setupPanels();

        const juce::StringArray tabNames { "INSTRUMENT", "RESONANCE", "AMBIENCE", "MOTION", "GHOST & EXPRESSION", "SCALE & TUNING" };
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

        stage.onPresetChosen = [this] { presetChanged(); };
        addAndMakeVisible (stage);
        addAndMakeVisible (logo);

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

        // Campo de estrelas no fundo do corpo (sempre igual)
        for (int i = 0; i < 220; ++i)
            bodyStars.push_back ({ 264.0f + random.nextFloat() * 1016.0f, 150.0f + random.nextFloat() * 650.0f,
                                   random.nextFloat() });

        const int savedTab = juce::jlimit (0, (int) tabs.size() - 1, (int) processor.parameters.state.getProperty ("uiTab", 0));
        tabs[(size_t) savedTab].setToggleState (true, juce::dontSendNotification);
        showPage (savedTab);

        // Estrelas cadentes por cima de tudo (passam pelo fundo e, suaves, pelos painéis)
        shootingStars.setGlassAreas ({ { sidebarArea, 0.45f }, { leftPanelArea, 0.35f }, { rightPanelArea, 0.35f },
                                       { stageArea, 0.6f }, { outputStripArea, 0.35f }, { macroArea, 0.3f },
                                       { patchDisplayArea, 0.25f }, { tabRowArea, 0.4f }, { keyboardArea, 0.0f } });
        addAndMakeVisible (shootingStars);

        starsButton.setButtonText ("STARS");
        starsButton.setClickingTogglesState (true);
        starsButton.setToggleState ((bool) processor.parameters.state.getProperty ("uiStars", true), juce::dontSendNotification);
        starsButton.onClick = [this]
        {
            const bool on = starsButton.getToggleState();
            processor.parameters.state.setProperty ("uiStars", on, nullptr);
            shootingStars.setActive (on);
        };
        shootingStars.setActive (starsButton.getToggleState());
        addAndMakeVisible (starsButton);

        // Seletor de tema (preferência do utilizador, vale para todos os projetos)
        themeSelector.addItemList (MugenTheme::names(), 1);
        themeSelector.setSelectedItemIndex (MugenTheme::current(), juce::dontSendNotification);
        themeSelector.onChange = [this]
        {
            const int id = themeSelector.getSelectedItemIndex();
            if (id < 0 || id == MugenTheme::current())
                return;
            MugenTheme::apply (id);
            MugenTheme::save (id);
            if (onThemeChanged)
                onThemeChanged();
        };
        addAndMakeVisible (themeSelector);

        presetChanged();
        startTimerHz (30);
    }

    ~MainView() override { stopTimer(); }

    void paint (juce::Graphics& g) override
    {
        // Corpo: fundo profundo com nebulosas e estrelas
        g.setColour (MugenColours::body);
        g.fillAll();
        g.setGradientFill (juce::ColourGradient (MugenColours::violet.withAlpha (0.10f), 520.0f, 300.0f,
                                                 MugenColours::violet.withAlpha (0.0f), 1100.0f, 300.0f, true));
        g.fillRect (getLocalBounds());
        g.setGradientFill (juce::ColourGradient (MugenColours::blue.withAlpha (0.07f), 1150.0f, 720.0f,
                                                 MugenColours::blue.withAlpha (0.0f), 1150.0f, 300.0f, true));
        g.fillRect (getLocalBounds());
        for (const auto& star : bodyStars)
        {
            g.setColour (juce::Colours::white.withAlpha (0.06f + 0.3f * star.z * star.z));
            const float size = 0.8f + 1.2f * star.z;
            g.fillEllipse (star.x, star.y, size, size);
        }

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
        g.drawText ("v" JucePlugin_VersionString, 1090, 0, 70, 40, juce::Justification::centredRight);
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
            g.setGradientFill (juce::ColourGradient (MugenColours::bandA, band.getX(), band.getY(),
                                                     MugenColours::bandB, band.getRight(), band.getBottom(), false));
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

            drawThemeArt (g, band);

            g.setColour (juce::Colours::black.withAlpha (0.35f));
            g.fillRect (band.withTop (band.getBottom() - 2.0f));

            // (o logótipo MUGEN 3D é desenhado pelo componente MugenLogo)

            g.setColour (juce::Colours::white.withAlpha (0.75f));
            g.setFont (font (11.0f, false, 0.6f));
            g.drawText ("SPACE DRUM", 290, 110, 220, 20, juce::Justification::centredLeft);
            g.drawText ("MADE BY MUGEN", 1030, 110, 230, 20, juce::Justification::centredRight);
        }

        // Faixa de saída
        drawPanel (g, outputStripArea, "OUT");

        // Brilho por trás do palco central
        g.setGradientFill (juce::ColourGradient (MugenColours::violet.withAlpha (0.16f), (float) stageArea.getCentreX(), (float) stageArea.getCentreY(),
                                                 MugenColours::violet.withAlpha (0.0f), (float) stageArea.getCentreX() + 420.0f, (float) stageArea.getCentreY(), true));
        g.fillRect (stageArea.expanded (40, 20));

        // Macros
        {
            const auto area = macroArea.toFloat();
            g.setGradientFill (juce::ColourGradient (MugenColours::panelTop, 0.0f, area.getY(),
                                                     MugenColours::panelBottom, 0.0f, area.getBottom(), false));
            g.fillRoundedRectangle (area, 12.0f);
            g.setColour (MugenColours::panelOutline);
            g.drawRoundedRectangle (area.reduced (0.5f), 12.0f, 1.0f);

        }
    }

    /** Arte do cabeçalho própria de cada tema. */
    void drawThemeArt (juce::Graphics& g, juce::Rectangle<float> band)
    {
        const int theme = MugenTheme::current();
        g.saveState();
        g.reduceClipRegion (band.toNearestInt());

        if (theme == MugenTheme::nebula)
        {
            // Planeta com anel, discreto, à direita
            const juce::Point<float> c (band.getRight() - 200.0f, band.getCentreY() - 6.0f);
            g.setGradientFill (juce::ColourGradient (MugenColours::violet.withAlpha (0.45f), c.x, c.y,
                                                     MugenColours::violet.withAlpha (0.0f), c.x + 90.0f, c.y, true));
            g.fillEllipse (juce::Rectangle<float> (180.0f, 180.0f).withCentre (c));
            g.setGradientFill (juce::ColourGradient (MugenColours::blue.brighter (0.3f), c.x - 10.0f, c.y - 12.0f,
                                                     MugenColours::bandA.darker (0.5f), c.x + 16.0f, c.y + 18.0f, true));
            g.fillEllipse (juce::Rectangle<float> (40.0f, 40.0f).withCentre (c));
            g.setColour (MugenColours::blue.withAlpha (0.7f));
            g.drawEllipse (juce::Rectangle<float> (86.0f, 20.0f).withCentre (c), 1.3f);
        }
        else if (theme == MugenTheme::aurora)
        {
            // Cortinas de aurora boreal a ondular pelo céu
            const juce::Colour colours[] = { MugenColours::violet, MugenColours::blue, MugenColours::starA };
            for (int r = 0; r < 3; ++r)
            {
                juce::Path curtain;
                const float phase = (float) r * 1.7f;
                const float topBase = band.getY() + 8.0f + (float) r * 10.0f;
                const float height = 64.0f - (float) r * 10.0f;

                for (float x = band.getX(); x <= band.getRight(); x += 6.0f)
                {
                    const float u = (x - band.getX()) / band.getWidth();
                    const float y = topBase + 12.0f * std::sin (u * 9.0f + phase) + 6.0f * std::sin (u * 23.0f + phase * 2.0f);
                    if (x == band.getX()) curtain.startNewSubPath (x, y);
                    else                  curtain.lineTo (x, y);
                }
                for (float x = band.getRight(); x >= band.getX(); x -= 6.0f)
                {
                    const float u = (x - band.getX()) / band.getWidth();
                    const float y = topBase + height + 12.0f * std::sin (u * 9.0f + phase) + 10.0f * std::sin (u * 5.0f - phase);
                    curtain.lineTo (x, y);
                }
                curtain.closeSubPath();

                juce::ColourGradient fill (colours[r].withAlpha (0.38f - 0.08f * (float) r), 0.0f, topBase,
                                           colours[r].withAlpha (0.0f), 0.0f, topBase + height + 10.0f, false);
                g.setGradientFill (fill);
                g.fillPath (curtain);
            }
        }
        else if (theme == MugenTheme::eclipse)
        {
            // Sol negro com coroa dourada e raios
            const juce::Point<float> c (band.getRight() - 200.0f, band.getCentreY() - 4.0f);
            constexpr float radius = 30.0f;
            g.setGradientFill (juce::ColourGradient (MugenColours::violet.withAlpha (0.65f), c.x, c.y,
                                                     MugenColours::blue.withAlpha (0.0f), c.x + 110.0f, c.y, true));
            g.fillEllipse (juce::Rectangle<float> (220.0f, 220.0f).withCentre (c));

            juce::Random rays (17);
            for (int i = 0; i < 48; ++i)
            {
                const float a = juce::MathConstants<float>::twoPi * (float) i / 48.0f;
                const float length = 10.0f + rays.nextFloat() * 26.0f;
                g.setColour (MugenColours::violet.withAlpha (0.18f + 0.25f * rays.nextFloat()));
                g.drawLine ({ c.getPointOnCircumference (radius + 3.0f, a), c.getPointOnCircumference (radius + 3.0f + length, a) }, 1.0f);
            }

            g.setColour (juce::Colours::black);
            g.fillEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (c));
            g.setColour (MugenColours::violet.brighter (0.4f));
            g.drawEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (c), 1.6f);
            // "Anel de diamante" do eclipse
            g.setColour (juce::Colours::white.withAlpha (0.9f));
            g.fillEllipse (juce::Rectangle<float> (5.0f, 5.0f).withCentre (c.getPointOnCircumference (radius, -0.7f)));

            // Brasas no horizonte
            g.setGradientFill (juce::ColourGradient (MugenColours::blue.withAlpha (0.35f), band.getX() + 120.0f, band.getBottom(),
                                                     MugenColours::blue.withAlpha (0.0f), band.getX() + 420.0f, band.getBottom() - 60.0f, true));
            g.fillRect (band);
        }
        else if (theme == MugenTheme::sakura)
        {
            // Lua cheia e pétalas de cerejeira a flutuar
            const juce::Point<float> c (band.getRight() - 200.0f, band.getCentreY() - 6.0f);
            g.setGradientFill (juce::ColourGradient (MugenColours::blue.withAlpha (0.5f), c.x, c.y,
                                                     MugenColours::blue.withAlpha (0.0f), c.x + 90.0f, c.y, true));
            g.fillEllipse (juce::Rectangle<float> (180.0f, 180.0f).withCentre (c));
            g.setGradientFill (juce::ColourGradient (juce::Colours::white, c.x - 10.0f, c.y - 12.0f,
                                                     MugenColours::blue, c.x + 22.0f, c.y + 22.0f, true));
            g.fillEllipse (juce::Rectangle<float> (52.0f, 52.0f).withCentre (c));
            g.setColour (MugenColours::blue.darker (0.4f).withAlpha (0.35f));
            g.fillEllipse (juce::Rectangle<float> (10.0f, 8.0f).withCentre (c.translated (-8.0f, 4.0f)));
            g.fillEllipse (juce::Rectangle<float> (6.0f, 6.0f).withCentre (c.translated (9.0f, -8.0f)));

            juce::Random petals (7);
            for (int i = 0; i < 34; ++i)
            {
                const juce::Point<float> p (band.getX() + petals.nextFloat() * band.getWidth(),
                                            band.getY() + petals.nextFloat() * band.getHeight());
                const float size = 4.0f + petals.nextFloat() * 6.0f;
                juce::Path petal;
                petal.addEllipse (-size, -size * 0.45f, size * 2.0f, size * 0.9f);
                petal.applyTransform (juce::AffineTransform::rotation (petals.nextFloat() * juce::MathConstants<float>::twoPi)
                                          .translated (p.x, p.y));
                g.setColour (MugenColours::violet.withAlpha (0.35f + 0.45f * petals.nextFloat()));
                g.fillPath (petal);
            }
        }

        g.restoreState();
    }

    /** Volta a aplicar as cores do tema aos componentes com cores próprias. */
    void applyThemeColours()
    {
        for (auto* k : allKnobs)
        {
            k->label.setColour (juce::Label::textColourId, MugenColours::text);
            styleValueBox (k->slider);
        }
        styleValueBox (volumeFader);
        for (auto& macro : macros)
            macro.label.setColour (juce::Label::textColourId, MugenColours::textDim);
        keyboard.refreshColours();
        browser.refreshColours();
        repaint();
    }

    std::function<void()> onThemeChanged;

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

        stage.setBounds (stageArea);
        logo.setBounds (518, 44, 508, 56);
        shootingStars.setBounds (getLocalBounds());
        starsButton.setBounds (1010, 9, 64, 22);
        themeSelector.setBounds (850, 9, 150, 22);

        // Faixa OUT: fader do volume + knob Width
        auto output = outputStripArea.reduced (4, 8).withTrimmedTop (30);
        auto widthArea = output.removeFromBottom (112);
        output.removeFromBottom (6);
        volumeLabel.setBounds (output.removeFromBottom (18));
        volumeFader.setBounds (output.reduced (0, 4));
        width.slider.setBounds (widthArea.removeFromTop (88).withSizeKeepingCentre (72, 88));
        width.label.setBounds (widthArea.removeFromTop (20));

        // Macros
        auto macroRow = macroArea.reduced (24, 6).withTrimmedLeft (10);
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
        allKnobs.push_back (&knob);
        knob.attachment = std::make_unique<SliderAttachment> (processor.parameters, id, knob.slider);
    }

    struct Macro
    {
        juce::String name;
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<SliderAttachment> attachment;
    };

    void makeMacro (Macro& macro, const char* id, const juce::String& name)
    {
        macro.slider.setSliderStyle (juce::Slider::LinearHorizontal);
        macro.slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        macro.name = name;
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

        auto& ghostPage = pages[4];
        fillPanel (ghostPage.left, "GHOST NOTES", "Hold notes: Ghost walks the scale, Arp plays them",
                   { { &ghostButton.button, &ghostModeSelector.box }, { &ghostRateSelector.box } }, { &ghostDensity, &ghostRange });
        fillPanel (ghostPage.right, "EXPRESSION", "Wheel & aftertouch open Shimmer + Motion",
                   {}, { &velocitySens, &wheelAmount, &pressureAmount });

        auto& scalePage = pages[5];
        fillPanel (scalePage.left, "SCALE", "Every key plays the scale",
                   { { &rootSelector.box, &mappingSelector.box }, { &scaleSelector.box } }, { &octave, &tune });
        fillPanel (scalePage.right, "SCALE MAP", "Notes of the chosen scale", {}, {}, &scaleCircle);

        for (auto& page : pages)
        {
            addChildComponent (page.left);
            addChildComponent (page.right);
        }

        // Faixa OUT
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
    }

    juce::String valueText (const char* id) const
    {
        if (auto* param = processor.parameters.getParameter (id))
            return param->getCurrentValueAsText();
        return {};
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

        // Valores dos macros ao lado do nome
        for (auto& macro : macros)
        {
            const auto text = macro.name + "   " + juce::String (juce::roundToInt (macro.slider.getValue() * 100.0 / juce::jmax (0.0001, macro.slider.getMaximum()))) + "%";
            if (macro.label.getText() != text)
                macro.label.setText (text, juce::dontSendNotification);
        }
    }

    MugenSpaceDrumProcessor& processor;

    Knob character, brightness, blend, resonance, decay,
         delayMix, delayTime, feedback, reverbMix, reverbSize, shimmer,
         depth, rate, width, octave, tune,
         ghostDensity, ghostRange, velocitySens, wheelAmount, pressureAmount;

    juce::Slider volumeFader;
    juce::Label volumeLabel;
    std::unique_ptr<SliderAttachment> volumeAttachment;

    std::array<Macro, 4> macros;

    Selector sampleSelector, divisionSelector, reverbSelector, rootSelector, scaleSelector, mappingSelector;
    Toggle syncButton, freezeButton, ghostButton;
    Selector ghostRateSelector, ghostModeSelector;

    PatchBrowser browser;
    PatchDisplay patchDisplay;
    OrbVisualizer orb;
    ScaleCircle scaleCircle;
    MotionScope motionScope;
    ScaleKeyboard keyboard;
    PresetStage stage;

    std::array<TabPanels, 6> pages;
    std::array<juce::TextButton, 6> tabs;

    ShootingStars shootingStars;
    juce::TextButton starsButton;
    struct BodyStar { float x, y, z; };
    std::vector<BodyStar> bodyStars;

    std::unique_ptr<juce::FileChooser> chooser;

    juce::Rectangle<int> ledBounds;
    float ledLevel = 0.0f;
    int currentPage = 0;
    juce::String lastPresetName;
    int lastScaleSignature = -1;
    std::vector<juce::Point<float>> stars;
    std::vector<Knob*> allKnobs;
    juce::ComboBox themeSelector;
    MugenLogo logo;
};

// ==============================================================================
MugenSpaceDrumEditor::MugenSpaceDrumEditor (MugenSpaceDrumProcessor& p)
    : AudioProcessorEditor (&p)
{
    // Tema guardado nas tuas preferências
    MugenTheme::apply (MugenTheme::loadSaved());
    lookAndFeel.refreshColours();
    setLookAndFeel (&lookAndFeel);

    view = std::make_unique<MainView> (p);
    view->onThemeChanged = [this]
    {
        lookAndFeel.refreshColours();
        view->applyThemeColours();
        sendLookAndFeelChange();
        repaint();
    };
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
