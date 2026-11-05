#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <atomic>

// ==============================================================================
//  Voice Engine + Sound Generator
//
//  Cada voz é um pequeno "tambor" feito por síntese modal: vários osciladores
//  senoidais (parciais) com afinações próprias e decaimentos independentes,
//  como as línguas de metal de um Space Drum / handpan.
//
//  - Parciais graves duram mais, parciais agudos desaparecem depressa.
//  - Resonance acrescenta um segundo oscilador ligeiramente desafinado em
//    cada parcial (batimento suave, som "vivo" e etéreo) e prolonga os agudos.
//  - Brightness controla quanto dos parciais agudos e do toque da baqueta
//    se ouve.
// ==============================================================================

/** Parâmetros que cada voz lê no momento em que a nota começa. */
struct VoiceParameters
{
    std::atomic<float>* brightness = nullptr;
    std::atomic<float>* decay      = nullptr;
    std::atomic<float>* resonance  = nullptr;
};

class SpaceDrumSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote (int) override    { return true; }
    bool appliesToChannel (int) override { return true; }
};

class SpaceDrumVoice : public juce::SynthesiserVoice
{
public:
    explicit SpaceDrumVoice (VoiceParameters params);

    bool canPlaySound (juce::SynthesiserSound*) override;
    void startNote (int midiNoteNumber, float velocity,
                    juce::SynthesiserSound*, int currentPitchWheelPosition) override;
    void stopNote (float velocity, bool allowTailOff) override;
    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}
    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer,
                          int startSample, int numSamples) override;

    static constexpr int numPartials = 6;

private:
    /** Oscilador senoidal por rotação (barato e muito limpo). */
    struct Oscillator
    {
        double cosState = 1.0, sinState = 0.0;
        double cosStep  = 1.0, sinStep  = 0.0;

        void setFrequency (double hz, double sampleRate)
        {
            const double w = juce::MathConstants<double>::twoPi * hz / sampleRate;
            cosStep = std::cos (w);
            sinStep = std::sin (w);
            cosState = 1.0;
            sinState = 0.0;
        }

        inline double next() noexcept
        {
            const double c = cosState * cosStep - sinState * sinStep;
            const double s = cosState * sinStep + sinState * cosStep;
            cosState = c;
            sinState = s;
            return s;
        }

        void normalise() noexcept
        {
            const double mag = std::sqrt (cosState * cosState + sinState * sinState);
            if (mag > 0.0)
            {
                cosState /= mag;
                sinState /= mag;
            }
        }
    };

    struct Partial
    {
        Oscillator main, shimmer;
        double mainAmp = 0.0, shimmerAmp = 0.0;
        double envelope = 0.0, envelopeMultiplier = 1.0;
    };

    VoiceParameters parameters;
    std::array<Partial, numPartials> partials;

    // Toque da baqueta (ruído filtrado muito curto)
    juce::Random random;
    double malletLevel = 0.0, malletMultiplier = 0.0, malletFilterCoef = 0.0, malletState = 0.0;

    // Ataque suave (evita cliques e deixa o som "fofinho")
    int attackSamples = 1, attackPosition = 0;

    // Filtro passa-baixo final da voz
    double toneFilterCoef = 1.0, toneFilterState = 0.0;

    // Ganho e posição estéreo
    double outputGain = 0.0;
    float gainLeft = 0.7f, gainRight = 0.7f;

    bool isActive = false;
};
