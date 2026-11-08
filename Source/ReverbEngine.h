#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <vector>

// ==============================================================================
//  Reverb Engine (Fase 4)
//
//  Reverb próprio do Mugen Space Drum, do tipo "Feedback Delay Network":
//  8 linhas de atraso que trocam energia entre si, com difusão de entrada,
//  modulação lenta (cauda mais suave e viva) e amortecimento dos agudos.
//
//  Tipos:  Room · Hall · Cathedral · Deep Space
//  Extras: Shimmer – parte da cauda é afinada uma oitava acima e volta a
//                    entrar no reverb (brilho celestial que sobe).
//          Freeze  – a cauda fica suspensa no ar, sem desaparecer.
// ==============================================================================
class ReverbEngine
{
public:
    enum Type { room = 0, hall, cathedral, deepSpace, numTypes };

    static juce::StringArray typeNames() { return { "Room", "Hall", "Cathedral", "Deep Space" }; }

    struct Settings
    {
        int   type    = hall;
        float size    = 0.8f;   // 0..1 dentro do tipo escolhido
        float mix     = 0.4f;   // 0..1
        float shimmer = 0.0f;   // 0..1
        bool  freeze  = false;
    };

    void prepare (double sampleRate);
    void reset();
    void process (float* left, float* right, int numSamples, const Settings& settings);

    /** Salta já para estas definições (limpa a cauda). Usado nas transições suaves. */
    void snap (const Settings& settings);

private:
    static constexpr int numLines = 8;
    static constexpr int numDiffusers = 4;

    /** Linha de atraso circular com leitura fracionária. */
    struct DelayLine
    {
        std::vector<float> buffer;
        int writeIndex = 0;

        void allocate (int size) { buffer.assign ((size_t) juce::jmax (4, size), 0.0f); writeIndex = 0; }
        void clear() { std::fill (buffer.begin(), buffer.end(), 0.0f); writeIndex = 0; }

        inline void push (float x) noexcept
        {
            buffer[(size_t) writeIndex] = x;
            if (++writeIndex >= (int) buffer.size())
                writeIndex = 0;
        }

        inline float read (float delaySamples) const noexcept
        {
            const int size = (int) buffer.size();
            float pos = (float) writeIndex - delaySamples;
            while (pos < 0.0f)
                pos += (float) size;
            const int i0 = (int) pos;
            const int i1 = (i0 + 1) % size;
            const float frac = pos - (float) i0;
            return buffer[(size_t) i0] + frac * (buffer[(size_t) i1] - buffer[(size_t) i0]);
        }
    };

    /** Allpass de difusão: espalha o ataque em muitos ecos muito próximos. */
    struct Diffuser
    {
        DelayLine line;
        float delay = 100.0f;
        float gain = 0.6f;

        inline float process (float x) noexcept
        {
            const float delayed = line.read (delay);
            const float v = x + gain * delayed;
            line.push (v);
            return delayed - gain * v;
        }
    };

    /** Desafinador de uma oitava (duas "janelas" cruzadas numa linha de atraso). */
    struct OctaveShifter
    {
        DelayLine line;
        float grainSamples = 4000.0f;
        float phase = 0.0f;

        float process (float x) noexcept;
    };

    void updateTargets (const Settings& settings);

    double currentSampleRate = 44100.0;

    std::array<DelayLine, numLines> lines;
    std::array<juce::SmoothedValue<float>, numLines> lineLengths;
    std::array<float, numLines> lineGain {};
    std::array<float, numLines> dampState {};
    std::array<float, numLines> lfoPhase {};

    std::array<Diffuser, numDiffusers> diffusersLeft, diffusersRight;
    DelayLine preDelayLeft, preDelayRight;
    juce::SmoothedValue<float> preDelaySamples;

    OctaveShifter shimmerShifter;
    float shimmerFilterLow = 0.0f, shimmerFilterHigh = 0.0f, shimmerFeedback = 0.0f;

    juce::SmoothedValue<float> wetGain, dryGain, inputGain, shimmerAmount, freezeAmount;
    float dampCoef = 0.3f, modDepthSamples = 8.0f, lfoIncrement = 0.0f;
    float shimmerLoopGain = 1.0f;   // menor em salas muito longas, para o shimmer nunca "explodir"

    int lastType = -1;
    float lastSize = -1.0f;

    // Transição sem "pitch": em mudanças grandes (tipo de sala ou tamanho), o reverb
    // desvanece, muda em silêncio e volta — em vez de esticar as linhas (o que desafina).
    int   changeFadeState = 0;   // 0 parado, 1 a desvanecer, 2 a voltar
    float changeFade = 1.0f;
    float changeFadeStep = 0.001f;
};
