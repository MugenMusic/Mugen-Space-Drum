#include "PluginProcessor.h"
#include "PluginEditor.h"

MugenSpaceDrumProcessor::MugenSpaceDrumProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "MugenSpaceDrum", createParameterLayout())
{
    VoiceParameters voiceParams;
    voiceParams.character  = parameters.getRawParameterValue (ParamIDs::character);
    voiceParams.brightness = parameters.getRawParameterValue (ParamIDs::brightness);
    voiceParams.decay      = parameters.getRawParameterValue (ParamIDs::decay);
    voiceParams.resonance  = parameters.getRawParameterValue (ParamIDs::resonance);
    voiceParams.sampleType = parameters.getRawParameterValue (ParamIDs::sampleType);
    voiceParams.blend      = parameters.getRawParameterValue (ParamIDs::blend);
    voiceParams.fineTune   = parameters.getRawParameterValue (ParamIDs::fineTune);
    voiceParams.sampleBank = &sampleBank;

    for (int i = 0; i < maxVoices; ++i)
        synth.addVoice (new SpaceDrumVoice (voiceParams));

    synth.addSound (new SpaceDrumSound());

    delayMix      = parameters.getRawParameterValue (ParamIDs::delayMix);
    delayTime     = parameters.getRawParameterValue (ParamIDs::delayTime);
    delayFeedback = parameters.getRawParameterValue (ParamIDs::delayFeedback);
    reverbMix     = parameters.getRawParameterValue (ParamIDs::reverbMix);
    reverbSize    = parameters.getRawParameterValue (ParamIDs::reverbSize);
    volume        = parameters.getRawParameterValue (ParamIDs::volume);
    delaySync     = parameters.getRawParameterValue (ParamIDs::delaySync);
    delayDivision = parameters.getRawParameterValue (ParamIDs::delayDivision);
    reverbType    = parameters.getRawParameterValue (ParamIDs::reverbType);
    shimmer       = parameters.getRawParameterValue (ParamIDs::shimmer);
    freeze        = parameters.getRawParameterValue (ParamIDs::freeze);
    motionDepth   = parameters.getRawParameterValue (ParamIDs::motionDepth);
    motionRate    = parameters.getRawParameterValue (ParamIDs::motionRate);
    width         = parameters.getRawParameterValue (ParamIDs::width);
    scaleRoot     = parameters.getRawParameterValue (ParamIDs::scaleRoot);
    scaleType     = parameters.getRawParameterValue (ParamIDs::scaleType);
    scaleMapping  = parameters.getRawParameterValue (ParamIDs::scaleMapping);
    octave        = parameters.getRawParameterValue (ParamIDs::octave);

    // Um plugin acabado de abrir começa no preset "Celestial Flow".
    // (Se o DAW tiver um estado guardado, esse estado substitui este logo a seguir.)
    presetManager.load (0);
}

void MugenSpaceDrumProcessor::prepareToPlay (double sampleRate, int)
{
    // Os samples orgânicos são "gravados" aqui, fora da thread de áudio,
    // e só quando a frequência de amostragem muda.
    if (! sampleBank.isReadyFor (sampleRate))
    {
        synth.allNotesOff (0, false);
        sampleBank.generate (sampleRate);
    }

    synth.setCurrentPlaybackSampleRate (sampleRate);
    scaleEngine.prepare();
    motion.prepare (sampleRate);
    ambience.prepare (sampleRate);
    keyboardState.reset();

    outputGain.reset (sampleRate, 0.05);
    outputGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (volume->load()));
}

void MugenSpaceDrumProcessor::releaseResources()
{
    motion.reset();
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

    // Notas -> notas da escala escolhida (Root, Scale, Mapping, Octave).
    scaleEngine.process (midiMessages,
                         juce::roundToInt (scaleRoot->load()),
                         juce::roundToInt (scaleType->load()),
                         juce::roundToInt (scaleMapping->load()),
                         juce::roundToInt (octave->load()));

    for (const auto metadata : midiMessages)
    {
        if (metadata.getMessage().isNoteOn())
        {
            midiActivity.store (true);
            notePulse.fetch_add (1);
            break;
        }
    }

    synth.renderNextBlock (buffer, midiMessages, 0, numSamples);

    // --- Motion (coro suave + movimento estéreo) -------------------------------
    MotionEngine::Settings motionSettings;
    motionSettings.depth  = motionDepth->load();
    motionSettings.rateHz = motionRate->load();
    motionSettings.width  = width->load();
    motion.processMotion (buffer, motionSettings);

    // --- Delay + Reverb ----------------------------------------------------------
    // Tempo do projeto (BPM) para o delay sincronizado.
    if (auto* host = getPlayHead())
        if (auto position = host->getPosition())
            if (auto bpm = position->getBpm())
                if (*bpm > 20.0 && *bpm < 400.0)
                    currentBpm = *bpm;

    AmbienceEngine::Settings settings;
    settings.delayMix      = delayMix->load();
    settings.delayFeedback = delayFeedback->load();

    if (delaySync->load() > 0.5f)
    {
        const float beats = DelayDivisions::beats (juce::roundToInt (delayDivision->load()));
        settings.delayTimeMs = juce::jmin (AmbienceEngine::maxDelayMs, (float) (60000.0 / currentBpm) * beats);
    }
    else
    {
        settings.delayTimeMs = delayTime->load();
    }

    settings.reverb.type    = juce::roundToInt (reverbType->load());
    settings.reverb.size    = reverbSize->load();
    settings.reverb.mix     = reverbMix->load();
    settings.reverb.shimmer = shimmer->load();
    settings.reverb.freeze  = freeze->load() > 0.5f;
    ambience.process (buffer, settings);

    motion.processWidth (buffer, motionSettings);

    // Volume final + limitação suave (evita distorção quando muitas notas soam juntas).
    outputGain.setTargetValue (juce::Decibels::decibelsToGain (volume->load()));

    float* left  = buffer.getWritePointer (0);
    float* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;

    float peak = 0.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        const float g = outputGain.getNextValue();
        left[i] = std::tanh (left[i] * g);
        peak = juce::jmax (peak, std::abs (left[i]));
        if (right != nullptr)
        {
            right[i] = std::tanh (right[i] * g);
            peak = juce::jmax (peak, std::abs (right[i]));
        }
    }
    outputLevel.store (peak);
}

int MugenSpaceDrumProcessor::getNumPrograms()
{
    return presetManager.getNumFactoryPresets();
}

int MugenSpaceDrumProcessor::getCurrentProgram()
{
    const int index = presetManager.getCurrentIndex();
    return (index >= 0 && index < presetManager.getNumFactoryPresets()) ? index : 0;
}

void MugenSpaceDrumProcessor::setCurrentProgram (int index)
{
    if (index >= 0 && index < presetManager.getNumFactoryPresets())
        presetManager.load (index);
}

const juce::String MugenSpaceDrumProcessor::getProgramName (int index)
{
    const auto& entries = presetManager.getEntries();
    return (index >= 0 && index < (int) entries.size()) ? entries[(size_t) index].name : juce::String();
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
