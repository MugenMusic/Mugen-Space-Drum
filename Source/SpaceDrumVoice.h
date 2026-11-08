#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <atomic>
#include "ModalModel.h"
#include "SampleBank.h"

// ==============================================================================
//  Voice Engine (Hybrid): Modal Synth Engine + Sample Engine
//
//  Cada voz é um pequeno corpo metálico feito por síntese modal: 10 modos de
//  vibração, cada um com afinação, volume e duração próprios.
//
//  - Character  : morph Tongue -> Handpan -> Crystal (ver ModalModel.h).
//  - Brightness : dureza da baqueta. Baqueta macia = poucos agudos e ataque
//                 redondo; baqueta dura = mais modos agudos e mais "toque".
//  - Resonance  : cada modo ganha um "gémeo" ligeiramente desafinado
//                 (batimento lento e etéreo) e os agudos duram mais.
//  - Decay      : duração geral das notas.
//
//  - Sample / Blend : mistura o motor modal com um sample orgânico gerado
//                 pelo próprio plugin (ver SampleBank.h). Blend 0 = só síntese,
//                 Blend 1 = só sample.
//
//  Detalhes orgânicos: ligeira descida de afinação no início de cada toque
//  (como num handpan real) e pequenas variações aleatórias em cada toque,
//  para que duas notas iguais nunca soem exatamente iguais.
// ==============================================================================

/** Parâmetros que cada voz lê no momento em que a nota começa. */
struct VoiceParameters
{
    std::atomic<float>* character  = nullptr;
    std::atomic<float>* brightness = nullptr;
    std::atomic<float>* decay      = nullptr;
    std::atomic<float>* resonance  = nullptr;
    std::atomic<float>* sampleType = nullptr;
    std::atomic<float>* blend      = nullptr;
    std::atomic<float>* fineTune   = nullptr;   // cêntimos (-50..+50)
    std::atomic<float>* velocitySens = nullptr; // 0 = todas as notas iguais, 1 = muito expressivo
    const SampleBank*   sampleBank = nullptr;
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

    static constexpr int numModes = ModalModel::numModes;

private:
    /** Oscilador senoidal por rotação (barato e muito limpo). */
    struct Oscillator
    {
        double cosState = 1.0, sinState = 0.0;
        double cosStep  = 1.0, sinStep  = 0.0;

        void reset() noexcept
        {
            cosState = 1.0;
            sinState = 0.0;
        }

        void setAngularStep (double radiansPerSample) noexcept
        {
            cosStep = std::cos (radiansPerSample);
            sinStep = std::sin (radiansPerSample);
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

    struct Mode
    {
        Oscillator main, twin;
        double mainStep = 0.0, twinStep = 0.0;   // radianos por amostra (sem glide)
        double mainAmp = 0.0, twinAmp = 0.0;
        double envelope = 0.0, envelopeMultiplier = 1.0;
    };

    void updateGlide() noexcept;
    double readSample() noexcept;

    VoiceParameters parameters;
    std::array<Mode, numModes> modes;

    juce::Random random;

    // Sample orgânico desta nota
    const SampleBank::Sample* currentSample = nullptr;
    double samplePosition = 0.0, sampleIncrement = 1.0;
    double synthMix = 1.0, sampleMix = 0.0;

    // Toque da baqueta (ruído filtrado muito curto)
    double malletLevel = 0.0, malletMultiplier = 0.0, malletFilterCoef = 0.0, malletState = 0.0;

    // Descida de afinação no ataque
    double glideAmount = 0.0, glideMultiplier = 1.0;
    int glideCounter = 0;
    static constexpr int glideUpdateInterval = 16;

    // Ataque suave
    int attackSamples = 1, attackPosition = 0;

    // Filtro passa-baixo final da voz
    double toneFilterCoef = 1.0, toneFilterState = 0.0;

    // Ganho e posição estéreo
    double outputGain = 0.0;
    float gainLeft = 0.7f, gainRight = 0.7f;

    bool isActive = false;

    // Anti-clique: quando uma voz é reutilizada (ou cortada pelo DAW), em vez de
    // um salto brusco, o som antigo desaparece em poucos milissegundos.
    float lastOutLeft = 0.0f, lastOutRight = 0.0f;
    float declickLeft = 0.0f, declickRight = 0.0f, declickCoef = 0.99f;
    bool  quickFade = false;
    float quickFadeGain = 1.0f, quickFadeCoef = 0.99f;
};
