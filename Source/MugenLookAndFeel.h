#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_data_structures/juce_data_structures.h>
#include <iterator>

// ==============================================================================
//  Identidade visual Mugen (v0.6.1)
//  Ardósia-índigo escura, acentos violeta e azul, knobs com anel luminoso,
//  faders em cápsula e painéis planos com cabeçalho — modernos e legíveis.
// ==============================================================================
namespace MugenColours
{
    // Cores ativas. Mudam quando se escolhe outro tema (ver MugenTheme).
    inline juce::Colour backgroundTop    { 0xff15112b };
    inline juce::Colour backgroundBottom { 0xff07060f };
    inline juce::Colour body             { 0xff111020 };
    inline juce::Colour sidebar          { 0xff0c0b17 };
    inline juce::Colour topBar           { 0xff08070f };
    inline juce::Colour panel            { 0xff17152b };
    inline juce::Colour panelHeader      { 0xff1e1b38 };
    inline juce::Colour panelOutline     { 0xff2c2852 };
    inline juce::Colour screen           { 0xff07060e };
    inline juce::Colour violet           { 0xff9b6bff };   // acento principal
    inline juce::Colour blue             { 0xff4fc3ff };   // acento secundário
    inline juce::Colour text             { 0xffe6e1ff };
    inline juce::Colour textDim          { 0xff8f88b8 };
    inline juce::Colour knobBody         { 0xff1b1736 };
    inline juce::Colour knobTrack        { 0xff2a2650 };
    inline juce::Colour panelTop         { 0xff1a1831 };
    inline juce::Colour panelBottom      { 0xff121024 };
    inline juce::Colour headerHighlight  { 0xff25214a };
    inline juce::Colour bandA            { 0xff2a1c6b };   // faixa do cabeçalho (esquerda)
    inline juce::Colour bandB            { 0xff17327a };   // faixa do cabeçalho (direita)
    inline juce::Colour chip             { 0xff141230 };
    inline juce::Colour keyWhite         { 0xffdcd8f2 };
    inline juce::Colour keyBlack         { 0xff15132a };
    inline juce::Colour keyLine          { 0xff3a3370 };
    inline juce::Colour listBackground   { 0xff0a0914 };
    inline juce::Colour displayEdge      { 0xff3b3577 };
    inline juce::Colour starA            { 0xff6ff3e0 };   // cores extra das estrelas cadentes
    inline juce::Colour starB            { 0xffff7ad9 };
    inline juce::Colour starC            { 0xffffd27a };
}

// ==============================================================================
//  Temas do Mugen Space Drum
//   0 Mugen Nebula    – violeta e azul, a identidade original
//   1 Aurora Borealis – noite polar, cortinas de aurora esmeralda e ciano
//   2 Solar Eclipse   – sol negro com coroa dourada e brasas de coral
//   3 Sakura Moon     – noite de cerejeiras, rosa sakura e prata de lua
//  O tema é uma preferência tua: fica guardado no Mac e vale para todos os projetos.
// ==============================================================================
namespace MugenTheme
{
    enum Id { nebula = 0, aurora, eclipse, sakura, numThemes };

    inline juce::StringArray names() { return { "Mugen Nebula", "Aurora Borealis", "Solar Eclipse", "Sakura Moon" }; }

    inline int& currentRef() { static int current = nebula; return current; }
    inline int current() { return currentRef(); }

    inline void apply (int id)
    {
        using namespace MugenColours;
        id = juce::jlimit (0, numThemes - 1, id);
        currentRef() = id;

        struct T { juce::uint32 c[29]; };
        static const T themes[] = {
            // bgTop     bgBottom    body        sidebar     topBar      panel       panelHdr    outline     screen      accentA     accentB     text        textDim     knobBody    knobTrack   panelTop    panelBot    hdrHi       bandA       bandB       chip        keyWhite    keyBlack    keyLine     listBg      dispEdge    starA       starB       starC
            {{ 0xff15112b, 0xff07060f, 0xff111020, 0xff0c0b17, 0xff08070f, 0xff17152b, 0xff1e1b38, 0xff2c2852, 0xff07060e, 0xff9b6bff, 0xff4fc3ff, 0xffe6e1ff, 0xff8f88b8, 0xff1b1736, 0xff2a2650, 0xff1a1831, 0xff121024, 0xff25214a, 0xff2a1c6b, 0xff17327a, 0xff141230, 0xffdcd8f2, 0xff15132a, 0xff3a3370, 0xff0a0914, 0xff3b3577, 0xff6ff3e0, 0xffff7ad9, 0xffffd27a }},
            {{ 0xff0b1a24, 0xff040a0f, 0xff08141b, 0xff061017, 0xff040b10, 0xff0e1f29, 0xff132a36, 0xff1f4250, 0xff030a0e, 0xff3ff2b0, 0xff52d6ff, 0xffdffbf4, 0xff7fa8a8, 0xff102530, 0xff1c3a45, 0xff11232e, 0xff0a1820, 0xff173441, 0xff0b3d45, 0xff16295e, 0xff0c2129, 0xffd6f3ee, 0xff0b1c22, 0xff2a5560, 0xff050e13, 0xff2a6a6a, 0xffa88bff, 0xffff8fc8, 0xfffff1a8 }},
            {{ 0xff1a1210, 0xff080504, 0xff120c0a, 0xff0d0907, 0xff070504, 0xff1b1310, 0xff261a14, 0xff4a3020, 0xff060403, 0xffffb547, 0xffff6a4d, 0xfffff1e0, 0xffb39a85, 0xff22170f, 0xff3a2618, 0xff1f1611, 0xff140d0a, 0xff33221a, 0xff2a1206, 0xff5a240a, 0xff1e130d, 0xfff3e6d6, 0xff1a110c, 0xff5a3a22, 0xff0a0605, 0xff7a4a22, 0xff9ad8ff, 0xffffe2a8, 0xffff9e7a }},
            {{ 0xff1d1226, 0xff0a0610, 0xff150e1c, 0xff0f0a15, 0xff09060d, 0xff1c1325, 0xff271a33, 0xff4a3060, 0xff08050c, 0xffff8fc8, 0xffc7d2ff, 0xfffdeaf5, 0xffb293ad, 0xff21162b, 0xff3a2648, 0xff21162b, 0xff150e1c, 0xff33213f, 0xff3d1a45, 0xff23285e, 0xff1c1226, 0xfff5e3ee, 0xff1a1020, 0xff5a3a6a, 0xff0b0710, 0xff7a4a80, 0xffffd1e8, 0xffb9f0ff, 0xffffe9b0 }},
        };

        const auto& t = themes[id].c;
        juce::Colour* targets[] = { &backgroundTop, &backgroundBottom, &body, &sidebar, &topBar, &panel, &panelHeader, &panelOutline,
                                    &screen, &violet, &blue, &text, &textDim, &knobBody, &knobTrack, &panelTop, &panelBottom,
                                    &headerHighlight, &bandA, &bandB, &chip, &keyWhite, &keyBlack, &keyLine, &listBackground,
                                    &displayEdge, &starA, &starB, &starC };
        for (size_t i = 0; i < std::size (targets); ++i)
            *targets[i] = juce::Colour (t[i]);
    }

    /** Ficheiro de preferências: ~/Library/Application Support/Mugen/MugenSpaceDrum.settings */
    inline std::unique_ptr<juce::PropertiesFile> openSettings()
    {
        juce::PropertiesFile::Options options;
        options.applicationName = "MugenSpaceDrum";
        options.filenameSuffix = ".settings";
        options.folderName = "Mugen";
        options.osxLibrarySubFolder = "Application Support";
        return std::make_unique<juce::PropertiesFile> (options);
    }

    inline int loadSaved()
    {
        auto settings = openSettings();
        return juce::jlimit (0, numThemes - 1, settings->getIntValue ("theme", nebula));
    }

    inline void save (int id)
    {
        auto settings = openSettings();
        settings->setValue ("theme", id);
        settings->saveIfNeeded();
    }
}

class MugenLookAndFeel : public juce::LookAndFeel_V4
{
public:
    MugenLookAndFeel()
    {
        refreshColours();
    }

    /** Volta a ler as cores do tema ativo. */
    void refreshColours()
    {
        setColour (juce::Slider::textBoxTextColourId, MugenColours::textDim);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxHighlightColourId, MugenColours::violet.withAlpha (0.4f));
        setColour (juce::Label::textColourId, MugenColours::text);
        setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);

        setColour (juce::ComboBox::backgroundColourId, MugenColours::screen);
        setColour (juce::ComboBox::outlineColourId, MugenColours::panelOutline);
        setColour (juce::ComboBox::textColourId, MugenColours::text);
        setColour (juce::ComboBox::arrowColourId, MugenColours::blue);
        setColour (juce::PopupMenu::backgroundColourId, MugenColours::panel);
        setColour (juce::PopupMenu::textColourId, MugenColours::text);
        setColour (juce::PopupMenu::headerTextColourId, MugenColours::violet);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, MugenColours::violet.withAlpha (0.45f));
        setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);

        setColour (juce::TextEditor::backgroundColourId, MugenColours::screen);
        setColour (juce::TextEditor::outlineColourId, MugenColours::panelOutline);
        setColour (juce::TextEditor::focusedOutlineColourId, MugenColours::violet);
        setColour (juce::TextEditor::textColourId, MugenColours::text);
        setColour (juce::TextEditor::highlightColourId, MugenColours::violet.withAlpha (0.4f));
        setColour (juce::CaretComponent::caretColourId, MugenColours::blue);

        setColour (juce::ScrollBar::thumbColourId, MugenColours::violet.withAlpha (0.45f));
        setColour (juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::ListBox::outlineColourId, juce::Colours::transparentBlack);
    }

    static juce::Font uiFont (float size, bool bold = false, float kerning = 0.0f)
    {
        return juce::Font (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain)
                               .withKerningFactor (kerning));
    }

    // --- Knobs ------------------------------------------------------------------
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float startAngle, float endAngle,
                           juce::Slider& slider) override
    {
        const auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (3.0f);
        const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const auto centre = bounds.getCentre();
        const float angle = startAngle + sliderPos * (endAngle - startAngle);
        const float alpha = slider.isEnabled() ? 1.0f : 0.4f;

        const float ringRadius = radius - 3.0f;
        const float bodyRadius = radius * 0.70f;

        // Marcas de início e fim
        for (auto a : { startAngle, endAngle })
        {
            const auto p1 = centre.getPointOnCircumference (radius - 0.5f, a);
            const auto p2 = centre.getPointOnCircumference (radius - 5.5f, a);
            g.setColour (MugenColours::textDim.withAlpha (0.5f * alpha));
            g.drawLine ({ p1, p2 }, 1.2f);
        }

        // Pista do anel
        juce::Path track;
        track.addCentredArc (centre.x, centre.y, ringRadius, ringRadius, 0.0f, startAngle, endAngle, true);
        g.setColour (MugenColours::knobTrack.withMultipliedAlpha (alpha));
        g.strokePath (track, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Anel do valor com brilho
        if (sliderPos > 0.002f)
        {
            juce::Path valueArc;
            valueArc.addCentredArc (centre.x, centre.y, ringRadius, ringRadius, 0.0f, startAngle, angle, true);

            juce::ColourGradient arcColour (MugenColours::violet.withMultipliedAlpha (alpha), bounds.getX(), bounds.getBottom(),
                                            MugenColours::blue.withMultipliedAlpha (alpha), bounds.getRight(), bounds.getY(), false);
            juce::ColourGradient glowColour (MugenColours::violet.withAlpha (0.22f * alpha), bounds.getX(), bounds.getBottom(),
                                             MugenColours::blue.withAlpha (0.22f * alpha), bounds.getRight(), bounds.getY(), false);

            g.setGradientFill (glowColour);
            g.strokePath (valueArc, juce::PathStrokeType (9.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setGradientFill (arcColour);
            g.strokePath (valueArc, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // Sombra do corpo
        g.setColour (juce::Colours::black.withAlpha (0.45f * alpha));
        g.fillEllipse (juce::Rectangle<float> (bodyRadius * 2.0f, bodyRadius * 2.0f).withCentre (centre.translated (0.0f, 2.5f)));

        // Corpo com luz de cima
        juce::ColourGradient bodyGradient (MugenColours::knobTrack.brighter (0.35f).withMultipliedAlpha (alpha), centre.x - bodyRadius * 0.5f, centre.y - bodyRadius,
                                           MugenColours::knobBody.darker (0.45f).withMultipliedAlpha (alpha), centre.x + bodyRadius * 0.4f, centre.y + bodyRadius, false);
        g.setGradientFill (bodyGradient);
        g.fillEllipse (juce::Rectangle<float> (bodyRadius * 2.0f, bodyRadius * 2.0f).withCentre (centre));

        // Aro fino e reflexo
        g.setColour (juce::Colours::white.withAlpha (0.10f * alpha));
        g.drawEllipse (juce::Rectangle<float> (bodyRadius * 2.0f, bodyRadius * 2.0f).withCentre (centre).reduced (0.5f), 1.0f);
        juce::Path highlight;
        highlight.addCentredArc (centre.x, centre.y, bodyRadius - 2.0f, bodyRadius - 2.0f, 0.0f,
                                 -juce::MathConstants<float>::pi * 0.65f, -juce::MathConstants<float>::pi * 0.15f, true);
        g.setColour (juce::Colours::white.withAlpha (0.12f * alpha));
        g.strokePath (highlight, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Ponteiro
        const auto inner = centre.getPointOnCircumference (bodyRadius * 0.30f, angle);
        const auto outer = centre.getPointOnCircumference (bodyRadius * 0.82f, angle);
        g.setColour (juce::Colours::white.withAlpha (0.92f * alpha));
        g.drawLine ({ inner, outer }, 2.6f);
    }

    // --- Faders em cápsula ----------------------------------------------------
    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float, float, juce::Slider::SliderStyle style,
                           juce::Slider& slider) override
    {
        const bool vertical = style == juce::Slider::LinearVertical;
        const auto area = juce::Rectangle<int> (x, y, width, height).toFloat();
        const float alpha = slider.isEnabled() ? 1.0f : 0.4f;

        juce::Rectangle<float> trackArea = vertical
            ? juce::Rectangle<float> (4.0f, area.getHeight()).withCentre (area.getCentre())
            : juce::Rectangle<float> (area.getWidth(), 4.0f).withCentre (area.getCentre());

        g.setColour (MugenColours::knobTrack.withMultipliedAlpha (alpha));
        g.fillRoundedRectangle (trackArea, 2.0f);

        juce::Rectangle<float> filled = vertical
            ? trackArea.withTop (sliderPos)
            : trackArea.withRight (sliderPos);

        g.setGradientFill (juce::ColourGradient (MugenColours::violet.withMultipliedAlpha (alpha), filled.getX(), filled.getBottom(),
                                                 MugenColours::blue.withMultipliedAlpha (alpha), filled.getRight(), filled.getY(), false));
        g.fillRoundedRectangle (filled, 2.0f);

        // Tampa do fader
        const auto capCentre = vertical ? juce::Point<float> (area.getCentreX(), sliderPos)
                                        : juce::Point<float> (sliderPos, area.getCentreY());
        const auto cap = vertical ? juce::Rectangle<float> (30.0f, 13.0f).withCentre (capCentre)
                                  : juce::Rectangle<float> (13.0f, 24.0f).withCentre (capCentre);

        g.setColour (MugenColours::blue.withAlpha (0.18f * alpha));
        g.fillRoundedRectangle (cap.expanded (4.0f), 7.0f);
        g.setGradientFill (juce::ColourGradient (MugenColours::knobTrack.brighter (0.45f), cap.getX(), cap.getY(),
                                                 MugenColours::knobBody.darker (0.2f), cap.getX(), cap.getBottom(), false));
        g.fillRoundedRectangle (cap, 4.0f);
        g.setColour (juce::Colours::white.withAlpha (0.18f));
        g.drawRoundedRectangle (cap.reduced (0.5f), 4.0f, 1.0f);

        g.setColour (MugenColours::blue.withMultipliedAlpha (alpha));
        if (vertical)
            g.fillRect (cap.withSizeKeepingCentre (cap.getWidth() - 10.0f, 1.6f));
        else
            g.fillRect (cap.withSizeKeepingCentre (1.6f, cap.getHeight() - 8.0f));
    }

    // --- Seletores ------------------------------------------------------------
    void drawComboBox (juce::Graphics& g, int width, int height, bool, int, int, int, int, juce::ComboBox& box) override
    {
        const auto area = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height);
        g.setColour (MugenColours::screen);
        g.fillRoundedRectangle (area, 6.0f);
        g.setColour (box.isMouseOver (true) ? MugenColours::violet.withAlpha (0.8f) : MugenColours::panelOutline);
        g.drawRoundedRectangle (area.reduced (0.5f), 6.0f, 1.0f);

        juce::Path chevron;
        const float cx = (float) width - 14.0f, cy = (float) height * 0.5f;
        chevron.startNewSubPath (cx - 4.0f, cy - 2.0f);
        chevron.lineTo (cx, cy + 2.0f);
        chevron.lineTo (cx + 4.0f, cy - 2.0f);
        g.setColour (MugenColours::blue.withAlpha (box.isEnabled() ? 1.0f : 0.4f));
        g.strokePath (chevron, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override { return uiFont (13.0f); }

    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override
    {
        label.setBounds (8, 1, box.getWidth() - 30, box.getHeight() - 2);
        label.setFont (getComboBoxFont (box));
        label.setJustificationType (juce::Justification::centred);
    }

    juce::Font getPopupMenuFont() override { return uiFont (14.0f); }

    // --- Botões ---------------------------------------------------------------
    //  "tab"   : separador com sublinhado luminoso
    //  "ghost" : botão discreto (barras de ferramentas)
    //  outros  : botão arredondado, violeta quando ligado
    void drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&,
                               bool isMouseOver, bool isButtonDown) override
    {
        const auto area = button.getLocalBounds().toFloat();

        if (button.getProperties().contains ("tab"))
        {
            if (button.getToggleState())
            {
                g.setGradientFill (juce::ColourGradient (MugenColours::violet.withAlpha (0.28f), 0.0f, area.getY(),
                                                         MugenColours::violet.withAlpha (0.05f), 0.0f, area.getBottom(), false));
                g.fillRoundedRectangle (area.reduced (1.0f), 5.0f);

                juce::ColourGradient underline (MugenColours::violet, area.getX(), 0.0f, MugenColours::blue, area.getRight(), 0.0f, false);
                g.setGradientFill (underline);
                g.fillRoundedRectangle (area.withTop (area.getBottom() - 3.0f).reduced (6.0f, 0.0f), 1.5f);
            }
            else
            {
                g.setColour (isMouseOver ? MugenColours::panelHeader.brighter (0.08f) : MugenColours::panel);
                g.fillRoundedRectangle (area.reduced (1.0f), 5.0f);
                g.setColour (MugenColours::blue.withAlpha (0.35f));
                g.fillRoundedRectangle (area.withTop (area.getBottom() - 2.0f).reduced (10.0f, 0.0f), 1.0f);
            }
            return;
        }

        if (button.getProperties().contains ("ghost"))
        {
            g.setColour (isButtonDown ? MugenColours::knobTrack : (isMouseOver ? MugenColours::panelHeader : MugenColours::panel));
            g.fillRoundedRectangle (area.reduced (0.5f), 6.0f);
            g.setColour (MugenColours::panelOutline);
            g.drawRoundedRectangle (area.reduced (0.5f), 6.0f, 1.0f);
            return;
        }

        const bool on = button.getToggleState();
        if (on)
        {
            g.setGradientFill (juce::ColourGradient (MugenColours::violet, area.getX(), 0.0f, MugenColours::blue.withAlpha (0.9f), area.getRight(), 0.0f, false));
            g.fillRoundedRectangle (area.reduced (0.5f), 6.0f);
        }
        else
        {
            g.setColour (isButtonDown ? MugenColours::knobTrack : (isMouseOver ? MugenColours::panelHeader : MugenColours::screen));
            g.fillRoundedRectangle (area.reduced (0.5f), 6.0f);
            g.setColour (MugenColours::panelOutline);
            g.drawRoundedRectangle (area.reduced (0.5f), 6.0f, 1.0f);
        }
    }

    void drawButtonText (juce::Graphics& g, juce::TextButton& button, bool, bool) override
    {
        const bool isTab = button.getProperties().contains ("tab");
        const bool on = button.getToggleState();

        g.setFont (uiFont (isTab ? 11.5f : 12.0f, true, 0.12f));
        g.setColour (on ? juce::Colours::white : (isTab ? MugenColours::textDim : MugenColours::text.withAlpha (0.85f)));
        g.drawText (button.getButtonText(), button.getLocalBounds().reduced (4, 0), juce::Justification::centred, true);
    }

    void drawScrollbar (juce::Graphics& g, juce::ScrollBar&, int x, int y, int width, int height,
                        bool isVertical, int thumbStart, int thumbSize, bool isMouseOver, bool) override
    {
        const auto thumb = isVertical ? juce::Rectangle<int> (x + width / 2 - 2, thumbStart, 4, thumbSize)
                                      : juce::Rectangle<int> (thumbStart, y + height / 2 - 2, thumbSize, 4);
        g.setColour (MugenColours::violet.withAlpha (isMouseOver ? 0.7f : 0.4f));
        g.fillRoundedRectangle (thumb.toFloat(), 2.0f);
    }
};
