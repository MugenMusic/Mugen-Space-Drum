#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <vector>

// ==============================================================================
//  Sample Engine — banco de samples orgânicos GERADOS pelo próprio plugin
//
//  Não usamos gravações de terceiros: quando o plugin arranca, "grava" os
//  seus próprios samples com modelos físicos detalhados (muito mais pesados
//  do que daria para calcular nota a nota em tempo real). Depois cada nota
//  toca esses samples afinados, misturados com o motor modal.
//
//  Tipos de sample:
//    Hand  – toque de mão/dedos: pancada grave da palma + pele + ressonância curta
//    Felt  – baqueta de feltro: ataque redondo e quente
//    Shell – casca metálica rica: dezenas de modos e batimentos lentos
//    Air   – sopro etéreo: ar a ressoar dentro do metal, quase um pad
// ==============================================================================
class SampleBank
{
public:
    enum Type { hand = 0, felt, shell, air, numTypes };

    static juce::StringArray typeNames() { return { "Hand", "Felt", "Shell", "Air" }; }

    /** Notas de referência gravadas para cada tipo (de oitava em oitava). */
    static constexpr std::array<int, 5> rootNotes { 36, 48, 60, 72, 84 };

    struct Sample
    {
        std::vector<float> data;
        int rootNote = 60;
    };

    /** Gera todos os samples. Só é chamado fora da thread de áudio. */
    void generate (double sampleRate);

    bool isReadyFor (double sampleRate) const noexcept { return generatedSampleRate == sampleRate; }

    /** Sample mais próximo da nota pedida (nunca devolve nullptr depois de generate). */
    const Sample* find (int type, int midiNote) const noexcept;

private:
    std::vector<float> renderHand  (double frequency, double sampleRate, int seed) const;
    std::vector<float> renderFelt  (double frequency, double sampleRate, int seed) const;
    std::vector<float> renderShell (double frequency, double sampleRate, int seed) const;
    std::vector<float> renderAir   (double frequency, double sampleRate, int seed) const;

    std::array<std::array<Sample, rootNotes.size()>, numTypes> samples;
    double generatedSampleRate = 0.0;
};
