#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <vector>

// ==============================================================================
//  Scale Engine (Fase 5 — Musical System)
//
//  Transforma as notas que chegam do teclado / DAW em notas da escala escolhida,
//  para que nunca haja notas "erradas" — como num handpan real, que só tem as
//  notas da sua afinação.
//
//  Mapeamento:
//    Nearest    – cada tecla toca a nota da escala mais próxima.
//    White Keys – as teclas brancas tocam a escala nota a nota (Dó = nota raiz),
//                 as pretas tocam a nota da escala mais próxima.
//
//  Octave desloca tudo em oitavas. Chromatic desliga as escalas.
// ==============================================================================
class ScaleEngine
{
public:
    enum Mapping { nearest = 0, whiteKeys };

    struct Scale
    {
        const char* name;
        std::vector<int> intervals;   // semitons a partir da nota raiz
    };

    static const std::vector<Scale>& scales();
    static juce::StringArray scaleNames();
    static const std::vector<std::pair<int, const char*>>& scaleGroups();   // títulos do menu
    static juce::StringArray rootNames()    { return { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }; }
    static juce::StringArray mappingNames() { return { "Nearest", "White Keys" }; }

    static constexpr int defaultScale = 1;   // Celtic Minor
    static constexpr int defaultRoot  = 2;   // D

    /** A nota pertence à escala? (usado para marcar as teclas no ecrã) */
    static bool isInScale (int midiNote, int root, int scaleIndex) noexcept;

    /** Nota de saída para uma nota de entrada. */
    static int mapNote (int midiNote, int root, int scaleIndex, int mapping, int octave) noexcept;

    void prepare();
    void reset();

    /** Reescreve as notas do bloco de MIDI. Sem alocações de memória. */
    void process (juce::MidiBuffer& midi, int root, int scaleIndex, int mapping, int octave);

private:
    juce::MidiBuffer output;

    // Para cada tecla premida, que nota foi realmente tocada
    // (assim o Note Off desliga sempre a nota certa, mesmo que a escala mude).
    std::array<std::array<int, 128>, 16> activeNotes {};
};
