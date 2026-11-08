#include "MugenArt.h"

namespace
{
    constexpr float pi     = juce::MathConstants<float>::pi;
    constexpr float twoPi  = juce::MathConstants<float>::twoPi;

    float easeOut (float x)
    {
        x = juce::jlimit (0.0f, 1.0f, x);
        return 1.0f - (1.0f - x) * (1.0f - x) * (1.0f - x);
    }

    juce::Point<float> polar (juce::Point<float> c, float radius, float angle)
    {
        return { c.x + radius * std::cos (angle), c.y + radius * std::sin (angle) };
    }

    juce::Path polygon (juce::Point<float> c, float radius, int sides, float rotation)
    {
        juce::Path p;
        for (int i = 0; i < sides; ++i)
        {
            const auto v = polar (c, radius, rotation + twoPi * (float) i / (float) sides);
            if (i == 0) p.startNewSubPath (v);
            else        p.lineTo (v);
        }
        p.closeSubPath();
        return p;
    }

    juce::Path circle (juce::Point<float> c, float radius)
    {
        juce::Path p;
        p.addEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (c));
        return p;
    }

    /** Estrela de 4 pontas (cintilação). */
    void sparkle (juce::Graphics& g, juce::Point<float> c, float size, juce::Colour colour, float alpha)
    {
        if (alpha <= 0.01f || size <= 0.5f)
            return;

        juce::Path star;
        star.startNewSubPath (c.x, c.y - size);
        star.quadraticTo (c.x + size * 0.12f, c.y - size * 0.12f, c.x + size, c.y);
        star.quadraticTo (c.x + size * 0.12f, c.y + size * 0.12f, c.x, c.y + size);
        star.quadraticTo (c.x - size * 0.12f, c.y + size * 0.12f, c.x - size, c.y);
        star.quadraticTo (c.x - size * 0.12f, c.y - size * 0.12f, c.x, c.y - size);
        star.closeSubPath();

        g.setGradientFill (juce::ColourGradient (colour.withAlpha (0.45f * alpha), c.x, c.y,
                                                 colour.withAlpha (0.0f), c.x + size * 2.2f, c.y, true));
        g.fillEllipse (juce::Rectangle<float> (size * 4.4f, size * 4.4f).withCentre (c));
        g.setColour (juce::Colours::white.withAlpha (alpha));
        g.fillPath (star);
    }
}

namespace MugenArt
{
// ------------------------------------------------------------------------------
void glowStroke (juce::Graphics& g, const juce::Path& path, juce::Colour colour, float alpha, float width)
{
    if (alpha <= 0.005f)
        return;

    const juce::PathStrokeType::JointStyle joint = juce::PathStrokeType::curved;
    g.setColour (colour.withAlpha (0.10f * alpha));
    g.strokePath (path, juce::PathStrokeType (width * 6.0f, joint, juce::PathStrokeType::rounded));
    g.setColour (colour.withAlpha (0.22f * alpha));
    g.strokePath (path, juce::PathStrokeType (width * 2.8f, joint, juce::PathStrokeType::rounded));
    g.setColour (colour.interpolatedWith (juce::Colours::white, 0.35f).withAlpha (0.95f * alpha));
    g.strokePath (path, juce::PathStrokeType (width, joint, juce::PathStrokeType::rounded));
}

// ------------------------------------------------------------------------------
void drawLogo3D (juce::Graphics& g, juce::Rectangle<float> area, const juce::String& text,
                 const Palette& palette, float time)
{
    // Letras como forma (para as podermos esculpir)
    // Palavras longas (ex. SINGULARITY) usam um espaçamento mais justo para manter as letras grandes
    const float kerning = text.length() > 7 ? 0.20f : 0.42f;
    const juce::Font font (juce::FontOptions (area.getHeight() * 0.8f, juce::Font::bold).withKerningFactor (kerning));
    juce::GlyphArrangement glyphs;
    glyphs.addLineOfText (font, text, 0.0f, 0.0f);
    juce::Path letters;
    glyphs.createPath (letters);

    auto bounds = letters.getBounds();
    if (bounds.isEmpty())
        return;

    const float scale = juce::jmin (area.getWidth() * 0.96f / bounds.getWidth(), area.getHeight() * 0.74f / bounds.getHeight());
    letters.applyTransform (juce::AffineTransform::translation (-bounds.getCentreX(), -bounds.getCentreY())
                                .scaled (scale)
                                .translated (area.getCentreX(), area.getCentreY() - area.getHeight() * 0.06f));
    bounds = letters.getBounds();

    const auto accentA = palette.accentA;
    const auto accentB = palette.accentB;

    // 1) Aura suave à volta das letras
    for (auto [width, alpha] : { std::pair<float, float> { 26.0f, 0.045f }, { 16.0f, 0.07f }, { 8.0f, 0.10f } })
    {
        g.setColour (accentA.withAlpha (alpha));
        g.strokePath (letters, juce::PathStrokeType (width, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // 2) Sombra projetada
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillPath (letters, juce::AffineTransform::translation (7.0f, 11.0f));

    // 3) Extrusão: camadas cada vez mais escuras dão profundidade real
    constexpr int depth = 16;
    const float dx = 0.42f, dy = 0.62f;
    for (int i = depth; i >= 1; --i)
    {
        const float t = (float) i / (float) depth;
        const auto colour = accentA.darker (0.35f).interpolatedWith (juce::Colours::black, 0.25f + 0.65f * t);
        g.setColour (colour);
        g.fillPath (letters, juce::AffineTransform::translation (dx * (float) i, dy * (float) i));
    }
    // aresta de luz ao fundo da extrusão
    g.setColour (accentB.withAlpha (0.35f));
    g.strokePath (letters, juce::PathStrokeType (0.8f), juce::AffineTransform::translation (dx * depth, dy * depth));

    // 4) Face da frente: cromado com horizonte (como metal polido a refletir o céu)
    juce::ColourGradient face (juce::Colours::white, 0.0f, bounds.getY(),
                               accentB.interpolatedWith (juce::Colours::white, 0.55f), 0.0f, bounds.getBottom(), false);
    face.addColour (0.46, palette.text.interpolatedWith (accentA, 0.12f));
    face.addColour (0.50, accentA.interpolatedWith (juce::Colours::white, 0.15f).darker (0.25f));
    face.addColour (0.75, accentA.interpolatedWith (accentB, 0.5f).interpolatedWith (juce::Colours::white, 0.35f));
    g.setGradientFill (face);
    g.fillPath (letters);

    // 5) Bisel: luz no topo das letras, sombra na base
    {
        g.saveState();
        g.reduceClipRegion (letters);
        g.setColour (juce::Colours::black.withAlpha (0.30f));
        g.strokePath (letters, juce::PathStrokeType (2.4f), juce::AffineTransform::translation (0.0f, -1.6f));
        g.setColour (juce::Colours::white.withAlpha (0.55f));
        g.strokePath (letters, juce::PathStrokeType (2.0f), juce::AffineTransform::translation (0.0f, 1.4f));

        // 6) Brilho que atravessa as letras a cada ~7 segundos
        const float period = 7.0f;
        const float phase = std::fmod (time, period) / period;
        if (phase < 0.32f)
        {
            const float p = phase / 0.32f;
            const float x = bounds.getX() - 90.0f + (bounds.getWidth() + 180.0f) * p;
            juce::Path beam;
            beam.startNewSubPath (x, bounds.getY() - 10.0f);
            beam.lineTo (x + 46.0f, bounds.getY() - 10.0f);
            beam.lineTo (x + 16.0f, bounds.getBottom() + 10.0f);
            beam.lineTo (x - 30.0f, bounds.getBottom() + 10.0f);
            beam.closeSubPath();

            juce::ColourGradient shine (juce::Colours::white.withAlpha (0.0f), x - 30.0f, 0.0f,
                                        juce::Colours::white.withAlpha (0.0f), x + 46.0f, 0.0f, false);
            shine.addColour (0.5, juce::Colours::white.withAlpha (0.85f * std::sin (p * pi)));
            g.setGradientFill (shine);
            g.fillPath (beam);
        }
        g.restoreState();
    }

    // Contorno fino e nítido
    g.setColour (juce::Colours::white.withAlpha (0.35f));
    g.strokePath (letters, juce::PathStrokeType (0.7f));

    // 7) Cintilações nas pontas das letras
    const juce::Point<float> points[] = {
        { bounds.getRight() - 4.0f, bounds.getY() + 3.0f },
        { bounds.getX() + bounds.getWidth() * 0.37f, bounds.getY() + 2.0f },
        { bounds.getX() + 3.0f, bounds.getBottom() - 4.0f }
    };
    for (int k = 0; k < 3; ++k)
    {
        const float twinkle = std::pow (juce::jmax (0.0f, std::sin (time * 1.1f + (float) k * 2.3f)), 14.0f);
        sparkle (g, points[k], 7.0f + 5.0f * twinkle, accentB, twinkle);
    }
}

// ------------------------------------------------------------------------------
void drawLightRays (juce::Graphics& g, juce::Point<float> c, float inner, float outer,
                    float time, float intensity, const Palette& palette)
{
    constexpr int numRays = 9;
    for (int i = 0; i < numRays; ++i)
    {
        const float fi = (float) i;
        const float angle = twoPi * fi / (float) numRays + time * 0.035f + 0.12f * std::sin (time * 0.21f + fi);
        const float spread = 0.07f + 0.05f * (0.5f + 0.5f * std::sin (time * 0.47f + fi * 1.7f));
        const float length = outer * (0.75f + 0.25f * std::sin (time * 0.33f + fi * 2.1f));

        juce::Path ray;
        ray.startNewSubPath (polar (c, inner, angle - spread * 0.4f));
        ray.lineTo (polar (c, length, angle - spread));
        ray.lineTo (polar (c, length, angle + spread));
        ray.lineTo (polar (c, inner, angle + spread * 0.4f));
        ray.closeSubPath();

        const auto colour = (i % 2 == 0 ? palette.accentA : palette.accentB);
        const float a = intensity * (0.10f + 0.06f * std::sin (time * 0.8f + fi));
        g.setGradientFill (juce::ColourGradient (colour.withAlpha (a), c.x, c.y, colour.withAlpha (0.0f), c.x + length, c.y, true));
        g.fillPath (ray);
    }
}

// ------------------------------------------------------------------------------
void drawGlassOrb (juce::Graphics& g, juce::Point<float> c, float r, float glow, const Palette& palette, bool frontLayer)
{
    const auto sphere = juce::Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (c);

    if (! frontLayer)
    {
        // Interior: profundidade e um núcleo de luz
        g.setGradientFill (juce::ColourGradient (palette.accentB.interpolatedWith (palette.dark, 0.45f).withAlpha (0.95f),
                                                 c.x - r * 0.25f, c.y - r * 0.3f,
                                                 palette.dark.withAlpha (0.98f), c.x + r * 0.9f, c.y + r * 0.9f, true));
        g.fillEllipse (sphere);

        g.setGradientFill (juce::ColourGradient (palette.accentA.withAlpha (0.18f + 0.25f * glow), c.x, c.y,
                                                 palette.accentA.withAlpha (0.0f), c.x + r * 0.9f, c.y, true));
        g.fillEllipse (sphere);
        return;
    }

    // Frente: aro de Fresnel, reflexo e brilho de vidro
    juce::ColourGradient rim (palette.accentA.withAlpha (0.25f), c.x, c.y - r,
                              palette.accentB.withAlpha (0.9f), c.x, c.y + r, false);
    g.setGradientFill (rim);
    g.drawEllipse (sphere.reduced (0.8f), 1.6f);
    g.setColour (palette.accentB.withAlpha (0.10f + 0.15f * glow));
    g.drawEllipse (sphere.reduced (2.0f), 6.0f);

    // Reflexo principal (janela de luz no topo)
    const auto highlight = juce::Rectangle<float> (r * 1.05f, r * 0.55f).withCentre ({ c.x - r * 0.18f, c.y - r * 0.55f });
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.34f), highlight.getCentreX(), highlight.getY(),
                                             juce::Colours::white.withAlpha (0.0f), highlight.getCentreX(), highlight.getBottom(), false));
    g.fillEllipse (highlight);

    // Pequeno brilho especular
    g.setColour (juce::Colours::white.withAlpha (0.75f));
    g.fillEllipse (juce::Rectangle<float> (r * 0.09f, r * 0.06f).withCentre ({ c.x - r * 0.42f, c.y - r * 0.52f }));

    // Luz refletida em baixo (caustica)
    const auto caustic = juce::Rectangle<float> (r * 0.9f, r * 0.25f).withCentre ({ c.x + r * 0.1f, c.y + r * 0.72f });
    g.setGradientFill (juce::ColourGradient (palette.accentB.withAlpha (0.22f + 0.2f * glow), caustic.getCentreX(), caustic.getCentreY(),
                                             palette.accentB.withAlpha (0.0f), caustic.getRight(), caustic.getCentreY(), true));
    g.fillEllipse (caustic);
}

// ------------------------------------------------------------------------------
void drawEmblem (juce::Graphics& g, juce::Point<float> c, float radius,
                 const juce::String& category, int seed, float time, float reveal,
                 float alpha, float pulse, const Palette& palette)
{
    const float e = easeOut (reveal);
    const float a = alpha * e;
    if (a <= 0.01f)
        return;

    juce::Random rnd (seed);
    const float spin = (1.0f - e) * 0.9f + rnd.nextFloat() * twoPi;   // cada preset tem a sua orientação
    const float r = radius * (0.72f + 0.28f * e) * (1.0f + 0.04f * pulse);
    const auto A = palette.accentA;
    const auto B = palette.accentB;

    // Anel exterior com quatro marcas
    glowStroke (g, circle (c, r), A, a * 0.45f, 0.9f);
    for (int k = 0; k < 4; ++k)
    {
        const auto p = polar (c, r, time * 0.05f + spin + (float) k * pi * 0.5f);
        juce::Path diamond = polygon (p, 3.2f, 4, 0.0f);
        g.setColour (B.withAlpha (a));
        g.fillPath (diamond);
    }

    if (category == "Celestial")
    {
        // Estrela {n/k} com órbitas e planetas
        const int n = 5 + rnd.nextInt (4);
        const int k = (n == 8) ? 3 : 2;
        juce::Path star;
        for (int i = 0; i <= n; ++i)
        {
            const int idx = (i * k) % n;
            const auto p = polar (c, r * 0.72f, -pi * 0.5f + twoPi * (float) idx / (float) n + time * 0.06f + spin);
            if (i == 0) star.startNewSubPath (p);
            else        star.lineTo (p);
        }
        star.closeSubPath();
        glowStroke (g, star, B, a * 0.9f, 1.2f);
        glowStroke (g, circle (c, r * 0.36f), A, a * 0.6f, 1.0f);

        g.setColour (A.withAlpha (0.22f * a));
        g.drawEllipse (juce::Rectangle<float> (r * 1.76f, r * 1.76f).withCentre (c), 0.8f);
        for (int j = 0; j < 3; ++j)
        {
            const auto p = polar (c, r * 0.88f, time * (0.16f + 0.05f * (float) j) + (float) j * 2.1f + spin);
            g.setGradientFill (juce::ColourGradient (B.withAlpha (0.6f * a), p.x, p.y, B.withAlpha (0.0f), p.x + 9.0f, p.y, true));
            g.fillEllipse (juce::Rectangle<float> (18.0f, 18.0f).withCentre (p));
            g.setColour (juce::Colours::white.withAlpha (a));
            g.fillEllipse (juce::Rectangle<float> (4.0f, 4.0f).withCentre (p));
        }
    }
    else if (category == "Organic")
    {
        // Lótus de duas camadas e um anel de sementes
        const int n = 6 + rnd.nextInt (3);
        for (int layer = 0; layer < 2; ++layer)
        {
            const float length = r * (layer == 0 ? 0.92f : 0.6f);
            const float width = length * 0.36f;
            const float r0 = r * 0.10f;
            const float base = (float) layer * pi / (float) n + time * (layer == 0 ? 0.03f : -0.045f) + spin;
            juce::Path petals;
            for (int i = 0; i < n; ++i)
            {
                juce::Path petal;
                petal.startNewSubPath (0.0f, -r0);
                petal.cubicTo (width, -r0 - length * 0.35f, width * 0.6f, -length * 0.85f, 0.0f, -length);
                petal.cubicTo (-width * 0.6f, -length * 0.85f, -width, -r0 - length * 0.35f, 0.0f, -r0);
                petal.closeSubPath();
                petal.applyTransform (juce::AffineTransform::rotation (base + twoPi * (float) i / (float) n).translated (c.x, c.y));
                petals.addPath (petal);
            }
            g.setColour ((layer == 0 ? B : A).withAlpha (0.06f * a));
            g.fillPath (petals);
            glowStroke (g, petals, layer == 0 ? B : A, a * (layer == 0 ? 0.75f : 0.95f), 1.0f);
        }
        for (int i = 0; i < n * 2; ++i)
        {
            const auto p = polar (c, r * 0.3f, twoPi * (float) i / (float) (n * 2) - time * 0.05f + spin);
            g.setColour (juce::Colours::white.withAlpha (0.7f * a));
            g.fillEllipse (juce::Rectangle<float> (2.4f, 2.4f).withCentre (p));
        }
    }
    else if (category == "Rhythmic")
    {
        // Pele do tambor: anéis que pulsam, marcas radiais e hexagrama
        for (int j = 0; j < 3; ++j)
        {
            const float rr = r * (0.3f + 0.17f * (float) j) + pulse * 6.0f * (float) (3 - j);
            glowStroke (g, circle (c, rr), j % 2 == 0 ? A : B, a * (0.5f + 0.3f * pulse), 0.9f);
        }
        juce::Path ticks;
        for (int i = 0; i < 32; ++i)
        {
            const float ang = twoPi * (float) i / 32.0f + time * 0.04f + spin;
            ticks.startNewSubPath (polar (c, r * (i % 4 == 0 ? 0.78f : 0.84f), ang));
            ticks.lineTo (polar (c, r * 0.92f, ang));
        }
        g.setColour (B.withAlpha (0.8f * a));
        g.strokePath (ticks, juce::PathStrokeType (1.1f));
        glowStroke (g, polygon (c, r * 0.6f, 3, -pi * 0.5f + time * 0.08f + spin), B, a * 0.8f, 1.0f);
        glowStroke (g, polygon (c, r * 0.6f, 3, pi * 0.5f - time * 0.08f + spin), A, a * 0.8f, 1.0f);
    }
    else if (category == "Atmosphere")
    {
        // Flor da vida e um anel ondulante como vento
        const float rc = r * 0.3f;
        juce::Path flower = circle (c, rc);
        for (int i = 0; i < 6; ++i)
            flower.addPath (circle (polar (c, rc, twoPi * (float) i / 6.0f + time * 0.04f + spin), rc));
        glowStroke (g, flower, B, a * 0.7f, 0.9f);

        juce::Path wave;
        for (int i = 0; i <= 180; ++i)
        {
            const float th = twoPi * (float) i / 180.0f;
            const float rr = r * 0.84f + 4.0f * std::sin (th * 8.0f + time * 0.8f) + 2.0f * std::sin (th * 3.0f - time * 0.5f);
            const auto p = polar (c, rr, th);
            if (i == 0) wave.startNewSubPath (p);
            else        wave.lineTo (p);
        }
        wave.closeSubPath();
        glowStroke (g, wave, A, a * 0.75f, 1.0f);
    }
    else if (category == "Crystal")
    {
        // Hexágonos encaixados, facetas e cintilações nos vértices
        for (int j = 0; j < 3; ++j)
        {
            const float rr = r * (0.85f - 0.24f * (float) j);
            const float rot = (float) j * pi / 6.0f + time * 0.05f * (j % 2 == 0 ? 1.0f : -1.0f) + spin;
            glowStroke (g, polygon (c, rr, 6, rot), j % 2 == 0 ? B : A, a * (0.9f - 0.15f * (float) j), 1.0f);

            if (j == 0)
            {
                juce::Path facets;
                for (int v = 0; v < 6; ++v)
                {
                    facets.startNewSubPath (c);
                    facets.lineTo (polar (c, rr, rot + twoPi * (float) v / 6.0f));
                }
                g.setColour (B.withAlpha (0.3f * a));
                g.strokePath (facets, juce::PathStrokeType (0.8f));

                for (int v = 0; v < 6; ++v)
                {
                    const float tw = std::pow (juce::jmax (0.0f, std::sin (time * 1.6f + (float) v * 1.3f)), 10.0f);
                    sparkle (g, polar (c, rr, rot + twoPi * (float) v / 6.0f), 4.0f + 5.0f * tw, B, tw * a);
                }
            }
        }
    }
    else if (category == "Sequences")
    {
        // Espiral de sementes (ângulo de ouro) com uma onda que viaja para fora
        constexpr int count = 96;
        constexpr float golden = 2.39996323f;
        for (int i = 1; i < count; ++i)
        {
            const float u = (float) i / (float) count;
            const auto p = polar (c, r * 0.86f * std::sqrt (u), (float) i * golden + time * 0.08f + spin);
            const float wave = 0.5f + 0.5f * std::sin (time * 2.2f - (float) i * 0.18f);
            const float size = 1.2f + 2.4f * u * (0.6f + 0.4f * wave);
            g.setColour (A.interpolatedWith (B, u).withAlpha (a * (0.35f + 0.65f * wave)));
            g.fillEllipse (juce::Rectangle<float> (size, size).withCentre (p));
        }
        for (int j = 0; j < 3; ++j)
        {
            juce::Path arc;
            const float start = time * 0.3f + (float) j * twoPi / 3.0f + spin;
            arc.addCentredArc (c.x, c.y, r * 0.95f, r * 0.95f, 0.0f, start, start + 0.9f, true);
            glowStroke (g, arc, B, a * 0.8f, 1.3f);
        }
    }
    else
    {
        // Mandala genérica (presets do utilizador)
        juce::Path arcs;
        for (int i = 0; i < 8; ++i)
        {
            const float ang = twoPi * (float) i / 8.0f + time * 0.04f + spin;
            arcs.addCentredArc (polar (c, r * 0.4f, ang).x, polar (c, r * 0.4f, ang).y, r * 0.4f, r * 0.4f, 0.0f,
                                ang + pi * 0.5f, ang + pi * 1.5f, true);
        }
        glowStroke (g, arcs, B, a * 0.75f, 1.0f);
        glowStroke (g, circle (c, r * 0.8f), A, a * 0.5f, 0.9f);
    }

    // Núcleo de luz e cintilação central
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (a * (0.55f + 0.45f * pulse)), c.x, c.y,
                                             A.withAlpha (0.0f), c.x + r * 0.22f, c.y, true));
    g.fillEllipse (juce::Rectangle<float> (r * 0.44f, r * 0.44f).withCentre (c));
    sparkle (g, c, r * 0.12f * (1.0f + 0.5f * pulse), B, a);
}
}
