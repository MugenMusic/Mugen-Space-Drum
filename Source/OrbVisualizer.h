#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <array>

class MugenSpaceDrumProcessor;

// ==============================================================================
//  Orb Visualizer (v0.8.1)
//
//  O coração visual do Mugen Space Drum:
//    - Raios de luz volumétricos que rodam devagar por trás.
//    - Anel de onda ao vivo (o som real) com mostrador.
//    - Esfera de vidro com aro de luz e reflexos.
//    - Dentro dela, o EMBLEMA do preset: geometria sagrada própria de cada
//      categoria e de cada preset. Ao mudar de preset, o antigo dissolve-se e
//      expande-se enquanto o novo se desenha, com um clarão suave.
//    - Partículas em órbita e poeira de luz a subir.
//    - Tudo pulsa com as notas e brilha com o volume.
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

    struct Particle { float radius, angle, speed, size, phase; };
    struct Mote     { float x, y, speed, sway, phase, life, duration, size; bool alive; };
    struct Emblem   { juce::String category; int seed = 0; };

    MugenSpaceDrumProcessor& processor;
    std::array<Particle, 40> particles {};
    std::array<Mote, 36> motes {};
    juce::Random random;

    Emblem current, previous;
    juce::String currentKey;
    float transition = 1.0f;   // 0 → 1 depois de mudar de preset
    float bloom = 0.0f;

    float time = 0.0f;
    float energy = 0.0f;
    float pulse = 0.0f;
    int lastPulseCount = 0;
};
