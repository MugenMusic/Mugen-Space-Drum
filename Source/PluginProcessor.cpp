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
    voiceParams.velocitySens = parameters.getRawParameterValue (ParamIDs::velocitySens);
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
    wheelAmount    = parameters.getRawParameterValue (ParamIDs::wheelAmount);
    pressureAmount = parameters.getRawParameterValue (ParamIDs::pressureAmount);
    ghostOn        = parameters.getRawParameterValue (ParamIDs::ghostOn);
    ghostRate      = parameters.getRawParameterValue (ParamIDs::ghostRate);
    ghostMode      = parameters.getRawParameterValue (ParamIDs::ghostMode);
    ghostDensity   = parameters.getRawParameterValue (ParamIDs::ghostDensity);
    ghostRange     = parameters.getRawParameterValue (ParamIDs::ghostRange);
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
    ghostEngine.prepare (sampleRate);
    expressionSmoothed.reset (sampleRate, 0.08);
    modWheel = pressure = 0.0f;
    heldNoteCount = 0;
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

    // --- Tempo do projeto (para o delay sincronizado e as Ghost Notes) ------------
    GhostEngine::HostTime hostTime;
    if (auto* host = getPlayHead())
    {
        if (auto position = host->getPosition())
        {
            if (auto bpm = position->getBpm())
                if (*bpm > 20.0 && *bpm < 400.0)
                    currentBpm = *bpm;

            hostTime.isPlaying = position->getIsPlaying();
            if (auto ppq = position->getPpqPosition())
            {
                hostTime.hasPosition = true;
                hostTime.ppqPosition = *ppq;
            }
        }
    }
    hostTime.bpm = currentBpm;

    // --- Expressão: roda de modulação (CC1) e aftertouch -----------------------------
    for (const auto metadata : midiMessages)
    {
        const auto message = metadata.getMessage();
        if (message.isController() && message.getControllerNumber() == 1)
            modWheel = (float) message.getControllerValue() / 127.0f;
        else if (message.isChannelPressure())
            pressure = (float) message.getChannelPressureValue() / 127.0f;
        else if (message.isAftertouch())
            pressure = juce::jmax (pressure * 0.5f, (float) message.getAfterTouchValue() / 127.0f);
        else if (message.isNoteOn())
            ++heldNoteCount;
        else if (message.isNoteOff())
            heldNoteCount = juce::jmax (0, heldNoteCount - 1);
    }

    // Sem teclas premidas, o aftertouch volta a zero (alguns teclados não o enviam).
    if (heldNoteCount == 0)
        pressure = 0.0f;

    expressionSmoothed.setTargetValue (juce::jlimit (0.0f, 1.0f, modWheel * wheelAmount->load()
                                                               + pressure * pressureAmount->load()));
    expressionSmoothed.skip (numSamples);
    const float expression = expressionSmoothed.getCurrentValue();
    expressionLevel.store (expression);

    // --- Ghost Notes: padrões generativos dentro da escala ----------------------------
    GhostEngine::Settings ghost;
    ghost.enabled      = ghostOn->load() > 0.5f;
    ghost.mode         = juce::roundToInt (ghostMode->load());
    ghost.beatsPerStep = GhostEngine::rateBeats (juce::roundToInt (ghostRate->load()));
    ghost.density      = ghostDensity->load();
    ghost.rangeOctaves = juce::roundToInt (ghostRange->load());
    ghost.root         = juce::roundToInt (scaleRoot->load());
    ghost.scale        = juce::roundToInt (scaleType->load());
    ghostEngine.process (midiMessages, numSamples, ghost, hostTime);
    ghostActive.store (ghost.enabled && heldNoteCount > 0);

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
    // A expressão abre o movimento (até ao máximo)
    const float baseDepth = motionDepth->load();
    motionSettings.depth  = baseDepth + (1.0f - baseDepth) * expression * 0.9f;
    motionSettings.rateHz = motionRate->load();
    motionSettings.width  = width->load();
    motion.processMotion (buffer, motionSettings);

    // --- Delay + Reverb ----------------------------------------------------------

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
    // ... e o brilho celestial do shimmer
    const float baseShimmer = shimmer->load();
    settings.reverb.shimmer = baseShimmer + (1.0f - baseShimmer) * expression * 0.85f;
    settings.reverb.freeze  = freeze->load() > 0.5f;
    ambience.process (buffer, settings);

    motion.processWidth (buffer, motionSettings);

    // Volume final + limitação suave (evita distorção quando muitas notas soam juntas).
    outputGain.setTargetValue (juce::Decibels::decibelsToGain (volume->load()));

    float* left  = buffer.getWritePointer (0);
    float* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;

    float peak = 0.0f;
    int scopeIndex = scopeWriteIndex.load (std::memory_order_relaxed);
    for (int i = 0; i < numSamples; ++i)
    {
        const float g = outputGain.getNextValue();
        left[i] = std::tanh (left[i] * g);
        peak = juce::jmax (peak, std::abs (left[i]));
        float mono = left[i];
        if (right != nullptr)
        {
            right[i] = std::tanh (right[i] * g);
            peak = juce::jmax (peak, std::abs (right[i]));
            mono = 0.5f * (left[i] + right[i]);
        }

        scopeBuffer[(size_t) scopeIndex].store (mono, std::memory_order_relaxed);
        scopeIndex = (scopeIndex + 1) % scopeSize;
    }
    scopeWriteIndex.store (scopeIndex, std::memory_order_relaxed);
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
