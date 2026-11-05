#include "PluginProcessor.h"
#include "PluginEditor.h"

MugenSpaceDrumProcessor::MugenSpaceDrumProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "MugenSpaceDrum", createParameterLayout())
{
    VoiceParameters voiceParams;
    voiceParams.brightness = parameters.getRawParameterValue (ParamIDs::brightness);
    voiceParams.decay      = parameters.getRawParameterValue (ParamIDs::decay);
    voiceParams.resonance  = parameters.getRawParameterValue (ParamIDs::resonance);

    for (int i = 0; i < maxVoices; ++i)
        synth.addVoice (new SpaceDrumVoice (voiceParams));

    synth.addSound (new SpaceDrumSound());

    delayMix      = parameters.getRawParameterValue (ParamIDs::delayMix);
    delayTime     = parameters.getRawParameterValue (ParamIDs::delayTime);
    delayFeedback = parameters.getRawParameterValue (ParamIDs::delayFeedback);
    reverbMix     = parameters.getRawParameterValue (ParamIDs::reverbMix);
    reverbSize    = parameters.getRawParameterValue (ParamIDs::reverbSize);
    volume        = parameters.getRawParameterValue (ParamIDs::volume);
}

void MugenSpaceDrumProcessor::prepareToPlay (double sampleRate, int)
{
    synth.setCurrentPlaybackSampleRate (sampleRate);
    ambience.prepare (sampleRate);
    keyboardState.reset();

    outputGain.reset (sampleRate, 0.05);
    outputGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (volume->load()));
}

void MugenSpaceDrumProcessor::releaseResources()
{
    ambience.reset();
}

bool MugenSpaceDrumProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void MugenSpaceDrumProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    buffer.clear();

    // Junta as notas tocadas no teclado do ecrã às notas vindas do DAW.
    keyboardState.processNextMidiBuffer (midiMessages, 0, numSamples, true);

    for (const auto metadata : midiMessages)
    {
        if (metadata.getMessage().isNoteOn())
        {
            midiActivity.store (true);
            break;
        }
    }

    synth.renderNextBlock (buffer, midiMessages, 0, numSamples);

    AmbienceEngine::Settings settings;
    settings.delayMix      = delayMix->load();
    settings.delayTimeMs   = delayTime->load();
    settings.delayFeedback = delayFeedback->load();
    settings.reverbMix     = reverbMix->load();
    settings.reverbSize    = reverbSize->load();
    ambience.process (buffer, settings);

    // Volume final + limitação suave (evita distorção quando muitas notas soam juntas).
    outputGain.setTargetValue (juce::Decibels::decibelsToGain (volume->load()));

    float* left  = buffer.getWritePointer (0);
    float* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;

    for (int i = 0; i < numSamples; ++i)
    {
        const float g = outputGain.getNextValue();
        left[i] = std::tanh (left[i] * g);
        if (right != nullptr)
            right[i] = std::tanh (right[i] * g);
    }
}

juce::AudioProcessorEditor* MugenSpaceDrumProcessor::createEditor()
{
    return new MugenSpaceDrumEditor (*this);
}

void MugenSpaceDrumProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // Guarda os valores dos knobs dentro do projeto do Ableton / Bitwig.
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    if (xml != nullptr)
        copyXmlToBinary (*xml, destData);
}

void MugenSpaceDrumProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (parameters.state.getType()))
        parameters.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MugenSpaceDrumProcessor();
}
