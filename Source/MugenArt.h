#pragma once

#include <juce_graphics/juce_graphics.h>

// ==============================================================================
//  Mugen Art (v0.8.1) — desenho dos elementos "premium" da interface
//
//  Funções de desenho puras (sem componentes), para poderem ser usadas pelo
//  orbe, pelo logótipo e pelas pré-visualizações:
//    - drawLogo3D   : MUGEN com extrusão, bisel, brilho que atravessa e cintilação.
//    - drawEmblem   : emblema de geometria sagrada próprio de cada preset.
//    - drawGlassOrb : esfera de vidro com aro de luz e reflexo.
//    - drawLightRays: raios de luz volumétricos e suaves.
// ==============================================================================
namespace MugenArt
{
    struct Palette
    {
        juce::Colour accentA, accentB, text, dark;
    };

    /** Desenha um traço com "bloom" (três passagens: halo largo, halo médio, linha nítida). */
    void glowStroke (juce::Graphics& g, const juce::Path& path, juce::Colour colour, float alpha, float width = 1.3f);

    /** Logótipo 3D. time em segundos (anima o brilho e a cintilação). */
    void drawLogo3D (juce::Graphics& g, juce::Rectangle<float> area, const juce::String& text,
                     const Palette& palette, float time);

    /** Emblema do preset. reveal 0..1 (entrada animada), alpha global, pulse 0..1 (notas). */
    void drawEmblem (juce::Graphics& g, juce::Point<float> centre, float radius,
                     const juce::String& category, int seed, float time, float reveal,
                     float alpha, float pulse, const Palette& palette);

    /** Esfera de vidro. frontLayer=false desenha o interior (antes do emblema),
        frontLayer=true desenha o aro e os reflexos (depois do emblema). glow 0..1 segue o som. */
    void drawGlassOrb (juce::Graphics& g, juce::Point<float> centre, float radius, float glow,
                       const Palette& palette, bool frontLayer);

    /** Raios de luz que rodam devagar por trás do orbe. */
    void drawLightRays (juce::Graphics& g, juce::Point<float> centre, float innerRadius, float outerRadius,
                        float time, float intensity, const Palette& palette);
}
