#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <array>

class MugenSpaceDrumProcessor;

// ==============================================================================
//  Orb Visualizer (Fase 6)
//
//  O orbe central do Mugen Space Drum: anéis de luz, uma flor de lótus,
//  partículas em órbita e uma onda de energia em baixo.
//    - Pulsa a cada nota tocada.
//    - Brilha mais quando o som está mais forte.
//    - A cor acompanha o Character (Tongue violeta → Crystal azul-gelo).
//    - Roda à velocidade do Motion Rate.
// ==============================================================================
class OrbVisualizer : public juce::Component,
                      private juce::Timer
{
public:
    explicit OrbVisualizer (MugenSpaceDrumProcessor& processor);
    ~OrbVisualizer() override;

    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;

    struct Particle
    {
        float radius, angle, speed, size, phase;
    };

    MugenSpaceDrumProcessor& processor;
    std::array<Particle, 48> particles {};

    float time = 0.0f;
    float energy = 0.0f;
    float pulse = 0.0f;
    int lastPulseCount = 0;
};
