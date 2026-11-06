#include "OrbVisualizer.h"
#include "PluginProcessor.h"
#include "MugenLookAndFeel.h"
#include "MugenArt.h"

OrbVisualizer::OrbVisualizer (MugenSpaceDrumProcessor& p)
    : processor (p), random (2024)
{
    setInterceptsMouseClicks (false, false);

    for (auto& particle : particles)
    {
        particle.radius = 0.55f + random.nextFloat() * 0.45f;
        particle.angle  = random.nextFloat() * juce::MathConstants<float>::twoPi;
        particle.speed  = (0.12f + random.nextFloat() * 0.4f) * (random.nextBool() ? 1.0f : -1.0f);
        particle.size   = 0.8f + random.nextFloat() * 1.8f;
        particle.phase  = random.nextFloat() * juce::MathConstants<float>::twoPi;
    }

    for (auto& m : motes)
        m.alive = false;

    startTimerHz (60);
}

OrbVisualizer::~OrbVisualizer()
{
    stopTimer();
}

void OrbVisualizer::timerCallback()
{
    constexpr float dt = 1.0f / 60.0f;
    const float rate = processor.parameters.getRawParameterValue (ParamIDs::motionRate)->load();
    time += dt * (0.4f + rate * 2.0f);

    // Energia: sobe depressa com o som, desce devagar
    const float level = juce::jlimit (0.0f, 1.0f, processor.outputLevel.load() * 2.5f);
    energy = juce::jmax (level, energy * 0.94f);

    const int count = processor.notePulse.load();
    if (count != lastPulseCount)
    {
        lastPulseCount = count;
        pulse = 1.0f;
    }
    pulse *= 0.92f;

    // Mudou de preset? O emblema antigo dissolve-se e o novo desenha-se
    const auto& entries = processor.presetManager.getEntries();
    const int index = processor.presetManager.getCurrentIndex();
    const juce::String name = processor.presetManager.getCurrentName();
    const juce::String category = (index >= 0 && index < (int) entries.size()) ? entries[(size_t) index].category : juce::String ("Custom");
    const juce::String key = category + "/" + name;

    if (key != currentKey)
    {
        const bool firstTime = currentKey.isEmpty();
        previous = current;
        current = { category, name.hashCode() };
        currentKey = key;
        transition = firstTime ? 1.0f : 0.0f;
        bloom = firstTime ? 0.0f : 1.0f;
    }
    transition = juce::jmin (1.0f, transition + dt / 1.4f);
    bloom *= 0.93f;

    for (auto& particle : particles)
        particle.angle += particle.speed * dt * (0.6f + rate * 3.0f + energy);

    // Poeira de luz a subir (mais quando há som)
    const float spawnChance = dt * (3.0f + 14.0f * energy);
    if (random.nextFloat() < spawnChance)
    {
        for (auto& m : motes)
        {
            if (m.alive)
                continue;
            const auto b = getLocalBounds().toFloat();
            m = { b.getCentreX() + (random.nextFloat() - 0.5f) * b.getHeight() * 0.9f,
                  b.getCentreY() + b.getHeight() * (0.15f + random.nextFloat() * 0.3f),
                  14.0f + random.nextFloat() * 22.0f, 4.0f + random.nextFloat() * 10.0f,
                  random.nextFloat() * juce::MathConstants<float>::twoPi, 0.0f,
                  2.5f + random.nextFloat() * 2.5f, 0.8f + random.nextFloat() * 1.8f, true };
            break;
        }
    }
    for (auto& m : motes)
    {
        if (! m.alive)
            continue;
        m.life += dt;
        m.y -= m.speed * dt;
        if (m.life >= m.duration)
            m.alive = false;
    }

    repaint();
}

void OrbVisualizer::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const auto centre = bounds.getCentre();
    const float baseRadius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.26f;
    const float breathe = 1.0f + 0.025f * std::sin (time * 1.1f) + 0.05f * pulse;
    const float radius = baseRadius * breathe;

    // Paleta: o acento principal acompanha o Character (corpo macio → cristal)
    const float character = processor.parameters.getRawParameterValue (ParamIDs::character)->load();
    const auto accentA = MugenColours::violet.interpolatedWith (MugenColours::blue, character * 0.6f);
    const auto accentB = MugenColours::blue.interpolatedWith (juce::Colours::white, character * 0.25f);
    const MugenArt::Palette palette { accentA, accentB, MugenColours::text, MugenColours::screen };
    const float glow = 0.35f + 0.65f * energy;

    // --- Halo profundo -------------------------------------------------------------
    g.setGradientFill (juce::ColourGradient (accentA.withAlpha (0.22f * glow + 0.06f), centre.x, centre.y,
                                             accentA.withAlpha (0.0f), centre.x + radius * 2.4f, centre.y, true));
    g.fillEllipse (juce::Rectangle<float> (radius * 4.8f, radius * 4.8f).withCentre (centre));

    // --- Raios de luz volumétricos --------------------------------------------------
    MugenArt::drawLightRays (g, centre, radius * 0.9f, radius * 2.6f, time, 0.55f + 0.6f * glow + 0.6f * bloom, palette);

    // --- Poeira de luz a subir ----------------------------------------------------
    for (const auto& m : motes)
    {
        if (! m.alive)
            continue;
        const float u = m.life / m.duration;
        const float a = std::sin (juce::MathConstants<float>::pi * u) * 0.7f;
        const float x = m.x + std::sin (m.phase + m.life * 1.3f) * m.sway;
        g.setColour (accentB.interpolatedWith (juce::Colours::white, 0.5f).withAlpha (a));
        g.fillEllipse (juce::Rectangle<float> (m.size, m.size).withCentre ({ x, m.y }));
    }

    // --- Anel de onda ao vivo (o som real à volta do orbe) ----------------------------
    {
        const int size = MugenSpaceDrumProcessor::scopeSize;
        const int points = 256;
        const int end = processor.scopeWriteIndex.load (std::memory_order_relaxed);
        const float ringRadius = radius * 1.45f;
        const float amplitude = radius * 0.30f;

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
        MugenArt::glowStroke (g, wave, accentB, 0.45f + 0.5f * glow, 1.1f);

        // Mostrador de marcas finas
        for (int t = 0; t < 72; ++t)
        {
            const float a = juce::MathConstants<float>::twoPi * (float) t / 72.0f + time * 0.05f;
            const float inner = ringRadius + amplitude + (t % 6 == 0 ? 5.0f : 8.0f);
            g.setColour (MugenColours::textDim.withAlpha (t % 6 == 0 ? 0.45f : 0.16f));
            g.drawLine ({ centre.getPointOnCircumference (inner, a), centre.getPointOnCircumference (ringRadius + amplitude + 11.0f, a) }, 1.0f);
        }
    }

    // --- Partículas em órbita -----------------------------------------------------
    for (const auto& particle : particles)
    {
        const float r = radius * (1.05f + particle.radius * 0.45f);
        const auto pos = centre.getPointOnCircumference (r, particle.angle);
        const float twinkle = 0.35f + 0.65f * (0.5f + 0.5f * std::sin (time * 3.0f + particle.phase));
        g.setColour (accentB.withAlpha (twinkle * (0.25f + 0.6f * glow)));
        g.fillEllipse (juce::Rectangle<float> (particle.size, particle.size).withCentre (pos));
    }

    // --- Esfera de vidro com o emblema do preset lá dentro ----------------------------
    const float sphereRadius = radius * 0.92f;
    MugenArt::drawGlassOrb (g, centre, sphereRadius, glow, palette, false);

    {
        g.saveState();
        juce::Path clip;
        clip.addEllipse (juce::Rectangle<float> (sphereRadius * 2.0f, sphereRadius * 2.0f).withCentre (centre));
        g.reduceClipRegion (clip);

        const float t = transition;
        const float emblemRadius = sphereRadius * 0.86f;

        // O antigo dissolve-se a expandir; o novo desenha-se do centro
        if (t < 1.0f && previous.category.isNotEmpty())
            MugenArt::drawEmblem (g, centre, emblemRadius * (1.0f + 0.45f * t), previous.category, previous.seed,
                                  time, 1.0f, 1.0f - t, pulse, palette);

        MugenArt::drawEmblem (g, centre, emblemRadius, current.category, current.seed, time, t, 1.0f, pulse, palette);
        g.restoreState();
    }

    MugenArt::drawGlassOrb (g, centre, sphereRadius, glow, palette, true);

    // --- Clarão ao mudar de preset ------------------------------------------------
    if (bloom > 0.01f)
    {
        const float r = radius * (1.0f + 1.6f * (1.0f - bloom));
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.35f * bloom), centre.x, centre.y,
                                                 accentB.withAlpha (0.0f), centre.x + r, centre.y, true));
        g.fillEllipse (juce::Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (centre));
        g.setColour (accentB.withAlpha (0.6f * bloom));
        g.drawEllipse (juce::Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (centre), 1.4f);
    }
}
