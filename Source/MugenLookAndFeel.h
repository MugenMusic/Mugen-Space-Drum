#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// ==============================================================================
//  Identidade visual Mugen (v0.6.1)
//  Ardósia-índigo escura, acentos violeta e azul, knobs com anel luminoso,
//  faders em cápsula e painéis planos com cabeçalho — modernos e legíveis.
// ==============================================================================
namespace MugenColours
{
    const juce::Colour backgroundTop    { 0xff15112b };
    const juce::Colour backgroundBottom { 0xff07060f };
    const juce::Colour body             { 0xff111020 };
    const juce::Colour sidebar          { 0xff0c0b17 };
    const juce::Colour topBar           { 0xff08070f };
    const juce::Colour panel            { 0xff17152b };
    const juce::Colour panelHeader      { 0xff1e1b38 };
    const juce::Colour panelOutline     { 0xff2c2852 };
    const juce::Colour screen           { 0xff07060e };
    const juce::Colour violet           { 0xff9b6bff };
    const juce::Colour blue             { 0xff4fc3ff };
    const juce::Colour text             { 0xffe6e1ff };
    const juce::Colour textDim          { 0xff8f88b8 };
    const juce::Colour knobBody         { 0xff1b1736 };
    const juce::Colour knobTrack        { 0xff2a2650 };
}

class MugenLookAndFeel : public juce::LookAndFeel_V4
{
public:
    MugenLookAndFeel()
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
        setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff141226));
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
        juce::ColourGradient bodyGradient (juce::Colour (0xff35315e).withMultipliedAlpha (alpha), centre.x - bodyRadius * 0.5f, centre.y - bodyRadius,
                                           juce::Colour (0xff121027).withMultipliedAlpha (alpha), centre.x + bodyRadius * 0.4f, centre.y + bodyRadius, false);
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
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3d3870), cap.getX(), cap.getY(),
                                                 juce::Colour (0xff17152d), cap.getX(), cap.getBottom(), false));
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
                g.setColour (isMouseOver ? juce::Colour (0xff221f40) : juce::Colour (0xff17152b));
                g.fillRoundedRectangle (area.reduced (1.0f), 5.0f);
                g.setColour (MugenColours::blue.withAlpha (0.35f));
                g.fillRoundedRectangle (area.withTop (area.getBottom() - 2.0f).reduced (10.0f, 0.0f), 1.0f);
            }
            return;
        }

        if (button.getProperties().contains ("ghost"))
        {
            g.setColour (isButtonDown ? juce::Colour (0xff2a2650) : (isMouseOver ? juce::Colour (0xff201d3c) : juce::Colour (0xff15132a)));
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
            g.setColour (isButtonDown ? juce::Colour (0xff2a2650) : (isMouseOver ? juce::Colour (0xff201d3c) : MugenColours::screen));
            g.fillRoundedRectangle (area.reduced (0.5f), 6.0f);
            g.setColour (MugenColours::panelOutline);
            g.drawRoundedRectangle (area.reduced (0.5f), 6.0f, 1.0f);
        }
    }

    void drawButtonText (juce::Graphics& g, juce::TextButton& button, bool, bool) override
    {
        const bool isTab = button.getProperties().contains ("tab");
        const bool on = button.getToggleState();

        g.setFont (uiFont (isTab ? 12.5f : 12.0f, true, 0.12f));
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
