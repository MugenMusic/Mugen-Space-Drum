#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// ==============================================================================
//  Identidade visual Mugen: fundo escuro, violeta e azul, knobs circulares.
// ==============================================================================
namespace MugenColours
{
    const juce::Colour backgroundTop    { 0xff15112b };
    const juce::Colour backgroundBottom { 0xff07060f };
    const juce::Colour panel            { 0x66221d45 };
    const juce::Colour panelOutline     { 0x553a3370 };
    const juce::Colour violet           { 0xff9b6bff };
    const juce::Colour blue             { 0xff4fc3ff };
    const juce::Colour text             { 0xffe6e1ff };
    const juce::Colour textDim          { 0xff8f88b8 };
    const juce::Colour knobBody         { 0xff1b1736 };
    const juce::Colour knobTrack        { 0xff2d2852 };
}

class MugenLookAndFeel : public juce::LookAndFeel_V4
{
public:
    MugenLookAndFeel()
    {
        setColour (juce::Slider::textBoxTextColourId, MugenColours::text);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::Label::textColourId, MugenColours::textDim);

        setColour (juce::ComboBox::backgroundColourId, MugenColours::knobBody);
        setColour (juce::ComboBox::outlineColourId, MugenColours::panelOutline);
        setColour (juce::ComboBox::textColourId, MugenColours::text);
        setColour (juce::ComboBox::arrowColourId, MugenColours::blue);
        setColour (juce::PopupMenu::backgroundColourId, MugenColours::knobBody);
        setColour (juce::PopupMenu::textColourId, MugenColours::text);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, MugenColours::violet.withAlpha (0.5f));
        setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);

        setColour (juce::TextButton::buttonColourId, MugenColours::knobBody);
        setColour (juce::TextButton::buttonOnColourId, MugenColours::violet);
        setColour (juce::TextButton::textColourOffId, MugenColours::textDim);
        setColour (juce::TextButton::textColourOnId, juce::Colours::white);
    }

    // Botões: separadores com sublinhado luminoso, nome do preset em "ecrã" escuro,
    // restantes botões arredondados (violeta quando ligados).
    void drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&,
                               bool isMouseOver, bool isButtonDown) override
    {
        const auto area = button.getLocalBounds().toFloat();

        if (button.getProperties().contains ("tab"))
        {
            if (button.getToggleState())
            {
                juce::ColourGradient underline (MugenColours::violet.withAlpha (0.0f), area.getX(), area.getBottom(),
                                                MugenColours::blue.withAlpha (0.0f), area.getRight(), area.getBottom(), false);
                underline.addColour (0.5, MugenColours::blue);
                g.setGradientFill (underline);
                g.fillRect (area.withTop (area.getBottom() - 2.0f));

                g.setColour (MugenColours::violet.withAlpha (0.10f));
                g.fillRect (area.withTrimmedBottom (2.0f));
            }
            else if (isMouseOver)
            {
                g.setColour (juce::Colours::white.withAlpha (0.04f));
                g.fillRect (area);
            }
            return;
        }

        if (button.getProperties().contains ("presetName"))
        {
            g.setColour (juce::Colour (0xff0b0918));
            g.fillRoundedRectangle (area, 8.0f);
            g.setColour (isMouseOver ? MugenColours::violet : MugenColours::panelOutline);
            g.drawRoundedRectangle (area.reduced (0.5f), 8.0f, 1.0f);

            // Pequena seta para indicar o menu
            juce::Path arrow;
            const float cx = area.getRight() - 16.0f, cy = area.getCentreY();
            arrow.addTriangle (cx - 4.0f, cy - 2.0f, cx + 4.0f, cy - 2.0f, cx, cy + 3.0f);
            g.setColour (MugenColours::blue);
            g.fillPath (arrow);
            return;
        }

        auto base = button.getToggleState() ? MugenColours::violet : MugenColours::knobBody;
        if (isButtonDown)      base = base.brighter (0.2f);
        else if (isMouseOver)  base = base.brighter (0.1f);

        g.setColour (base);
        g.fillRoundedRectangle (area.reduced (0.5f), 6.0f);
        g.setColour (MugenColours::panelOutline);
        g.drawRoundedRectangle (area.reduced (0.5f), 6.0f, 1.0f);
    }

    void drawButtonText (juce::Graphics& g, juce::TextButton& button, bool, bool) override
    {
        const bool isTab = button.getProperties().contains ("tab");
        const bool isPreset = button.getProperties().contains ("presetName");

        g.setFont (juce::Font (juce::FontOptions (isPreset ? 15.0f : 12.0f,
                                                  isPreset ? juce::Font::plain : juce::Font::bold)
                                   .withKerningFactor (isPreset ? 0.02f : 0.12f)));

        const bool on = button.getToggleState();
        g.setColour (isTab ? (on ? MugenColours::text : MugenColours::textDim)
                           : (isPreset ? MugenColours::text : (on ? juce::Colours::white : MugenColours::textDim)));

        g.drawText (button.getButtonText(), button.getLocalBounds().reduced (isPreset ? 28 : 4, 0),
                    juce::Justification::centred, true);
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float startAngle, float endAngle,
                           juce::Slider&) override
    {
        const auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (6.0f);
        const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const auto centre = bounds.getCentre();
        const float angle = startAngle + sliderPos * (endAngle - startAngle);
        const float arcRadius = radius - 4.0f;

        // Corpo do knob
        const float bodyRadius = radius - 10.0f;
        juce::ColourGradient bodyGradient (MugenColours::knobBody.brighter (0.25f), centre.x, centre.y - bodyRadius,
                                           MugenColours::knobBody.darker (0.4f), centre.x, centre.y + bodyRadius, false);
        g.setGradientFill (bodyGradient);
        g.fillEllipse (centre.x - bodyRadius, centre.y - bodyRadius, bodyRadius * 2.0f, bodyRadius * 2.0f);

        // Pista do arco
        juce::Path track;
        track.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, startAngle, endAngle, true);
        g.setColour (MugenColours::knobTrack);
        g.strokePath (track, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Arco do valor (violeta -> azul) com brilho suave
        if (sliderPos > 0.001f)
        {
            juce::Path valueArc;
            valueArc.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, startAngle, angle, true);

            juce::ColourGradient arcGradient (MugenColours::violet, bounds.getX(), bounds.getBottom(),
                                              MugenColours::blue, bounds.getRight(), bounds.getY(), false);

            juce::ColourGradient glowGradient (MugenColours::violet.withAlpha (0.25f), bounds.getX(), bounds.getBottom(),
                                               MugenColours::blue.withAlpha (0.25f), bounds.getRight(), bounds.getY(), false);

            g.setGradientFill (glowGradient);
            g.strokePath (valueArc, juce::PathStrokeType (10.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            g.setGradientFill (arcGradient);
            g.strokePath (valueArc, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // Ponteiro
        const auto dot = centre.getPointOnCircumference (bodyRadius - 7.0f, angle);
        g.setColour (MugenColours::text);
        g.fillEllipse (dot.x - 3.0f, dot.y - 3.0f, 6.0f, 6.0f);
    }
};
