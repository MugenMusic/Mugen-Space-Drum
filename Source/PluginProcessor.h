#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "Parameters.h"
#include "SpaceDrumVoice.h"
#include "AmbienceEngine.h"
#include "SampleBank.h"
#include "MotionEngine.h"
#include "ScaleEngine.h"

// ==============================================================================
//  PluginProcessor
//  O "cérebro" áudio do plugin: recebe o MIDI do DAW, toca as vozes,
//  aplica delay + reverb e envia o som de volta para o DAW.
// ==============================================================================
class MugenSpaceDrumProcessor : public juce::AudioProcessor
{
public:
    MugenSpaceDrumProcessor();
    ~MugenSpaceDrumProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 25.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static constexpr int maxVoices = 16;

    juce::AudioProcessorValueTreeState parameters;
    juce::MidiKeyboardState keyboardState;     // teclado no ecrã do plugin
    std::atomic<bool> midiActivity { false };  // acende a luz MIDI na interface

private:
    SampleBank sampleBank;
    juce::Synthesiser synth;
    ScaleEngine scaleEngine;
    MotionEngine motion;
    AmbienceEngine ambience;
    juce::SmoothedValue<float> outputGain;

    std::atomic<float>* delayMix      = nullptr;
    std::atomic<float>* delayTime     = nullptr;
    std::atomic<float>* delayFeedback = nullptr;
    std::atomic<float>* reverbMix     = nullptr;
    std::atomic<float>* reverbSize    = nullptr;
    std::atomic<float>* volume        = nullptr;
    std::atomic<float>* delaySync     = nullptr;
    std::atomic<float>* delayDivision = nullptr;
    std::atomic<float>* reverbType    = nullptr;
    std::atomic<float>* shimmer       = nullptr;
    std::atomic<float>* freeze        = nullptr;
    std::atomic<float>* motionDepth   = nullptr;
    std::atomic<float>* motionRate    = nullptr;
    std::atomic<float>* width         = nullptr;
    std::atomic<float>* scaleRoot     = nullptr;
    std::atomic<float>* scaleType     = nullptr;
    std::atomic<float>* scaleMapping  = nullptr;
    std::atomic<float>* octave        = nullptr;

    double currentBpm = 120.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MugenSpaceDrumProcessor)
};
