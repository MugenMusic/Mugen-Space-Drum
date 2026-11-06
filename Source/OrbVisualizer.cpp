#include "OrbVisualizer.h"
#include "PluginProcessor.h"
#include "MugenLookAndFeel.h"

OrbVisualizer::OrbVisualizer (MugenSpaceDrumProcessor& p)
    : processor (p)
{
    setInterceptsMouseClicks (false, false);

    juce::Random random (2024);
    for (auto& particle : particles)
    {
        particle.radius = 0.55f + random.nextFloat() * 0.45f;
        particle.angle  = random.nextFloat() * juce::MathConstants<float>::twoPi;
        particle.speed  = (0.15f + random.nextFloat() * 0.5f) * (random.nextBool() ? 1.0f : -1.0f);
        particle.size   = 1.0f + random.nextFloat() * 2.2f;
        particle.phase  = random.nextFloat() * juce::MathConstants<float>::twoPi;
    }

    startTimerHz (60);
}

OrbVisualizer::~OrbVisualizer()
{
    stopTimer();
}

void OrbVisualizer::timerCallback()
{
    const float rate = processor.parameters.getRawParameterValue (ParamIDs::motionRate)->load();
    const float dt = 1.0f / 60.0f;
    time += dt * (0.4f + rate * 2.0f);

    // Energia: sobe depressa com o som, desce devagar.
    const float level = juce::jlimit (0.0f, 1.0f, processor.outputLevel.load() * 2.5f);
    energy = juce::jmax (level, energy * 0.94f);

    const int count = processor.notePulse.load();
    if (count != lastPulseCount)
    {
        lastPulseCount = count;
        pulse = 1.0f;
    }
    pulse *= 0.92f;

    for (auto& particle : particles)
        particle.angle += particle.speed * dt * (0.6f + rate * 3.0f + energy);

    repaint();
}

void OrbVisualizer::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const auto centre = bounds.getCentre();
    const float baseRadius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.22f;
    const float breathe = 1.0f + 0.03f * std::sin (time * 1.3f) + 0.08f * pulse;
    const float radius = baseRadius * breathe;

    const float character = processor.parameters.getRawParameterValue (ParamIDs::character)->load();
    const auto mainColour = MugenColours::violet.interpolatedWith (MugenColours::blue, character);
    const auto accent     = MugenColours::blue.interpolatedWith (juce::Colour (0xffd6f4ff), character * 0.6f);
    const float glow = 0.35f + 0.65f * energy;

    // --- Halo de fundo ----------------------------------------------------------
    {
        juce::ColourGradient halo (mainColour.withAlpha (0.35f * glow + 0.08f), centre.x, centre.y,
                                   mainColour.withAlpha (0.0f), centre.x + radius * 2.2f, centre.y, true);
        g.setGradientFill (halo);
        g.fillEllipse (juce::Rectangle<float> (radius * 4.4f, radius * 4.4f).withCentre (centre));
    }

    // --- Anel de onda ao vivo (o som real à volta do orbe) ----------------------------
    {
        const int size = MugenSpaceDrumProcessor::scopeSize;
        const int points = 256;
        const int end = processor.scopeWriteIndex.load (std::memory_order_relaxed);
        const float ringRadius = radius * 1.55f;
        const float amplitude = radius * 0.40f;

        juce::Path wave;
        for (int i = 0; i <= points; ++i)
        {
            const int k = i % points;
            const int index = (end - points * 2 + k * 2 + size * 4) % size;
            const float v = juce::jlimit (-1.0f, 1.0f, processor.scopeBuffer[(size_t) index].load (std::memory_order_relaxed) * 2.2f);
            const float a = juce::MathConstants<float>::twoPi * (float) k / (float) points - juce::MathConstants<float>::halfPi;
            const auto p = centre.getPointOnCircumference (ringRadius + v * amplitude, a);
            if (i == 0) wave.startNewSubPath (p);
            else        wave.lineTo (p);
        }
        wave.closeSubPath();

        juce::ColourGradient ringColour (MugenColours::violet.withAlpha (0.9f), centre.x - ringRadius, centre.y,
                                         accent.withAlpha (0.9f), centre.x + ringRadius, centre.y, false);
        g.setColour (accent.withAlpha (0.10f + 0.25f * glow));
        g.strokePath (wave, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved));
        g.setGradientFill (ringColour);
        g.strokePath (wave, juce::PathStrokeType (1.4f, juce::PathStrokeType::curved));

        // Marcas finas à volta (mostrador)
        for (int t = 0; t < 72; ++t)
        {
            const float a = juce::MathConstants<float>::twoPi * (float) t / 72.0f + time * 0.05f;
            const float inner = ringRadius + amplitude + (t % 6 == 0 ? 6.0f : 9.0f);
            const auto p1 = centre.getPointOnCircumference (inner, a);
            const auto p2 = centre.getPointOnCircumference (ringRadius + amplitude + 12.0f, a);
            g.setColour (MugenColours::textDim.withAlpha (t % 6 == 0 ? 0.45f : 0.18f));
            g.drawLine ({ p1, p2 }, 1.0f);
        }
    }

    // --- Partículas em órbita ---------------------------------------------------
    for (const auto& particle : particles)
    {
        const float r = radius * (1.15f + particle.radius * 0.9f);
        const auto pos = centre.getPointOnCircumference (r, particle.angle);
        const float twinkle = 0.35f + 0.65f * (0.5f + 0.5f * std::sin (time * 3.0f + particle.phase));
        g.setColour (accent.withAlpha (twinkle * (0.25f + 0.6f * glow)));
        g.fillEllipse (juce::Rectangle<float> (particle.size, particle.size).withCentre (pos));
    }

    // --- Anéis ----------------------------------------------------------------
    for (int ring = 0; ring < 3; ++ring)
    {
        const float r = radius * (1.0f + 0.16f * (float) ring);
        g.setColour (mainColour.withAlpha (0.18f + 0.1f * glow - 0.04f * (float) ring));
        g.drawEllipse (juce::Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (centre), 1.2f);

        // Arco luminoso a rodar em cada anel
        juce::Path arc;
        const float start = time * (0.6f + 0.35f * (float) ring) * (ring % 2 == 0 ? 1.0f : -1.0f);
        arc.addCentredArc (centre.x, centre.y, r, r, 0.0f, start, start + 0.9f + 0.4f * glow, true);
        g.setColour (accent.withAlpha (0.5f + 0.4f * glow));
        g.strokePath (arc, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // --- Esfera ---------------------------------------------------------------
    {
        juce::ColourGradient sphere (accent.withAlpha (0.55f * glow + 0.15f), centre.x, centre.y - radius * 0.3f,
                                     MugenColours::backgroundBottom.withAlpha (0.9f), centre.x, centre.y + radius, true);
        g.setGradientFill (sphere);
        g.fillEllipse (juce::Rectangle<float> (radius * 1.7f, radius * 1.7f).withCentre (centre));
    }

    // --- Flor de lótus --------------------------------------------------------
    {
        const float petal = radius * (0.38f + 0.06f * pulse);
        const int numPetals = 7;
        juce::Path lotus;

        for (int i = 0; i < numPetals; ++i)
        {
            // Pétalas em leque, como um lótus visto de frente
            const float spread = juce::MathConstants<float>::pi * 0.85f;
            const float a = -juce::MathConstants<float>::halfPi - spread * 0.5f + spread * (float) i / (float) (numPetals - 1);
            const float length = petal * (i == numPetals / 2 ? 1.25f : (1.0f - 0.12f * std::abs ((float) (i - numPetals / 2))));

            juce::Path p;
            p.startNewSubPath (0.0f, 0.0f);
            p.cubicTo (length * 0.45f, -length * 0.35f, length * 0.9f, -length * 0.22f, length, 0.0f);
            p.cubicTo (length * 0.9f, length * 0.22f, length * 0.45f, length * 0.35f, 0.0f, 0.0f);
            p.applyTransform (juce::AffineTransform::rotation (a)
                                  .translated (centre.x, centre.y + petal * 0.35f));
            lotus.addPath (p);
        }

        g.setColour (accent.withAlpha (0.25f + 0.35f * glow));
        g.strokePath (lotus, juce::PathStrokeType (5.0f));
        g.setColour (juce::Colours::white.withAlpha (0.75f + 0.25f * pulse));
        g.strokePath (lotus, juce::PathStrokeType (1.4f));
    }

}
