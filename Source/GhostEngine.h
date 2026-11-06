#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>

// ==============================================================================
//  Ghost Engine (v0.7) — sequenciador generativo
//
//  Enquanto seguras uma ou mais notas, o plugin toca sozinho notas "fantasma"
//  à volta delas, sempre dentro da escala escolhida e no tempo do DAW:
//    - Rate    : de quanto em quanto tempo pode nascer uma nota (1/4 ... 1/16).
//    - Density : probabilidade de cada passo tocar (pausas criam respiração).
//    - Range   : até quantas oitavas acima as notas podem passear (1 a 3).
//
//  Modos Arp: as notas seguradas tocam por ordem (a subir, a descer ou a subir
//  e descer), repetidas por Range oitavas.
//
//  O padrão Ghost é um "passeio" suave pela escala (passos pequenos, às vezes um
//  salto), com velocidades a variar como mãos humanas.
//  Sem alocações de memória durante o áudio.
// ==============================================================================
class GhostEngine
{
public:
    struct Settings
    {
        bool  enabled = false;
        int   mode = 0;              // 0 Ghost (generativo), 1 Arp Up, 2 Arp Down, 3 Arp Up/Down
        float beatsPerStep = 0.5f;   // 0.5 = colcheia
        float density = 0.6f;        // 0..1
        int   rangeOctaves = 2;      // 1..3
        int   root = 2;
        int   scale = 1;
    };

    struct HostTime
    {
        bool   isPlaying = false;
        bool   hasPosition = false;
        double ppqPosition = 0.0;    // posição em batidas (semínimas)
        double bpm = 120.0;
    };

    static juce::StringArray modeNames()  { return { "Ghost", "Arp Up", "Arp Down", "Arp Up/Down" }; }
    static juce::StringArray rateNames()  { return { "1/4", "1/8T", "1/8", "1/16T", "1/16" }; }
    static float rateBeats (int index)
    {
        static constexpr float values[] = { 1.0f, 1.0f / 3.0f, 0.5f, 1.0f / 6.0f, 0.25f };
        return values[juce::jlimit (0, 4, index)];
    }

    void prepare (double sampleRate);
    void reset();

    /** Lê as notas seguradas no bloco e acrescenta as notas fantasma. */
    void process (juce::MidiBuffer& midi, int numSamples, const Settings& settings, const HostTime& host);

private:
    void triggerStep (juce::MidiBuffer& out, int samplePosition, const Settings& settings);
    void triggerArpStep (juce::MidiBuffer& out, int samplePosition, const Settings& settings);
    void addNote (juce::MidiBuffer& out, int note, float velocity, int samplePosition);
    void flushNoteOffs (juce::MidiBuffer& out, int numSamples);
    int  buildScaleNotes (int lowest, int highest, const Settings& settings);

    static constexpr int ghostChannel = 16;   // canal próprio: nunca se confunde com as tuas notas
    static constexpr int maxPending = 64;

    double sampleRate = 44100.0;
    juce::Random random;
    juce::MidiBuffer output;

    // Notas que estás a segurar (por número de nota) e a sua força
    std::array<float, 128> heldVelocity {};
    int numHeld = 0;

    // Note Offs agendados das notas fantasma
    struct Pending { int note = -1; int samplesLeft = 0; };
    std::array<Pending, maxPending> pending {};

    std::array<int, 64> scaleNotes {};

    double freeRunBeats = 0.0;   // relógio próprio quando o DAW está parado
    int walkNote = -1;           // última nota do passeio
    int lastStepIndex = -1;
    int arpCounter = 0;          // posição no arpejo
};
