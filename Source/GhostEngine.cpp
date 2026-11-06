#include "GhostEngine.h"
#include "ScaleEngine.h"

void GhostEngine::prepare (double newSampleRate)
{
    sampleRate = newSampleRate;
    output.ensureSize (8192);
    reset();
}

void GhostEngine::reset()
{
    heldVelocity.fill (0.0f);
    numHeld = 0;
    for (auto& p : pending)
        p = {};
    freeRunBeats = 0.0;
    walkNote = -1;
    lastStepIndex = -1;
    arpCounter = 0;
}

void GhostEngine::addNote (juce::MidiBuffer& out, int note, float velocity, int samplePosition)
{
    out.addEvent (juce::MidiMessage::noteOn (ghostChannel, note, velocity), samplePosition);

    // Note Off no fim do passo (as vozes continuam a ressoar naturalmente)
    const int gate = juce::jmax (1, (int) (sampleRate * 0.12));
    for (auto& p : pending)
    {
        if (p.note < 0)
        {
            p.note = note;
            p.samplesLeft = samplePosition + gate;
            break;
        }
    }
}

void GhostEngine::triggerArpStep (juce::MidiBuffer& out, int samplePosition, const Settings& settings)
{
    // Notas seguradas, da mais grave para a mais aguda, repetidas por oitavas
    int sequence[128];
    int length = 0;
    const int octaves = juce::jlimit (1, 3, settings.rangeOctaves);
    for (int octave = 0; octave < octaves; ++octave)
        for (int n = 0; n < 128; ++n)
            if (heldVelocity[(size_t) n] > 0.0f && n + 12 * octave <= 127 && length < 128)
                sequence[length++] = n + 12 * octave;

    if (length == 0)
        return;

    int index = 0;
    const int step = arpCounter++;
    if (settings.mode == 1)                 // Up
        index = step % length;
    else if (settings.mode == 2)            // Down
        index = length - 1 - (step % length);
    else                                    // Up/Down (sem repetir as pontas)
    {
        const int period = juce::jmax (1, 2 * length - 2);
        const int p = step % period;
        index = p < length ? p : period - p;
    }

    // Density também cria pausas no arpejo (sem perder o lugar)
    if (random.nextFloat() > settings.density)
        return;

    const int note = sequence[juce::jlimit (0, length - 1, index)];
    const int base = note % 12;
    float strength = 0.0f;
    for (int octave = 0; octave <= 10; ++octave)
        if (base + 12 * octave < 128)
            strength = juce::jmax (strength, heldVelocity[(size_t) (base + 12 * octave)]);

    const bool accent = (step % length) == 0;
    const float velocity = juce::jlimit (0.08f, 1.0f, (strength > 0.0f ? strength : 0.7f)
                                                       * (accent ? 0.95f : 0.65f + 0.2f * random.nextFloat()));
    addNote (out, note, velocity, samplePosition);
}

int GhostEngine::buildScaleNotes (int lowest, int highest, const Settings& settings)
{
    int count = 0;
    for (int n = juce::jmax (0, lowest); n <= juce::jmin (127, highest) && count < (int) scaleNotes.size(); ++n)
        if (ScaleEngine::isInScale (n, settings.root, settings.scale))
            scaleNotes[(size_t) count++] = n;
    return count;
}

void GhostEngine::triggerStep (juce::MidiBuffer& out, int samplePosition, const Settings& settings)
{
    if (numHeld == 0)
        return;

    if (settings.mode > 0)
    {
        triggerArpStep (out, samplePosition, settings);
        return;
    }

    if (random.nextFloat() > settings.density)
        return;

    // Uma das notas seguradas serve de âncora
    int anchors[128];
    int numAnchors = 0;
    for (int n = 0; n < 128; ++n)
        if (heldVelocity[(size_t) n] > 0.0f)
            anchors[numAnchors++] = n;
    if (numAnchors == 0)
        return;

    const int anchor = anchors[random.nextInt (numAnchors)];
    const int count = buildScaleNotes (anchor - 5, anchor + 12 * juce::jlimit (1, 3, settings.rangeOctaves), settings);
    if (count == 0)
        return;

    // Posição atual do passeio dentro da escala
    if (walkNote < 0 || walkNote < scaleNotes[0] || walkNote > scaleNotes[(size_t) (count - 1)])
        walkNote = anchor;

    int position = 0;
    for (int i = 0; i < count; ++i)
        if (std::abs (scaleNotes[(size_t) i] - walkNote) < std::abs (scaleNotes[(size_t) position] - walkNote))
            position = i;

    // Passo: quase sempre pequeno, às vezes um salto, de vez em quando volta à âncora
    const float r = random.nextFloat();
    int stepSize;
    if (r < 0.12f)       stepSize = 0;                                     // repete (efeito eco)
    else if (r < 0.72f)  stepSize = random.nextBool() ? 1 : -1;            // vizinha
    else if (r < 0.92f)  stepSize = random.nextBool() ? 2 : -2;            // terça da escala
    else                 stepSize = (random.nextBool() ? 1 : -1) * (3 + random.nextInt (3));   // salto

    if (random.nextFloat() < 0.08f)
        position = 0;   // volta ao grave
    else
        position = juce::jlimit (0, count - 1, position + stepSize);

    walkNote = scaleNotes[(size_t) position];

    // Força: segue a nota segurada, com variação humana e acentos suaves
    const float base = heldVelocity[(size_t) anchor];
    const float humanize = 0.55f + 0.35f * random.nextFloat();
    const float velocity = juce::jlimit (0.08f, 1.0f, base * humanize);

    addNote (out, walkNote, velocity, samplePosition);
}

void GhostEngine::flushNoteOffs (juce::MidiBuffer& out, int numSamples)
{
    for (auto& p : pending)
    {
        if (p.note < 0)
            continue;

        if (p.samplesLeft < numSamples)
        {
            out.addEvent (juce::MidiMessage::noteOff (ghostChannel, p.note), juce::jmax (0, p.samplesLeft));
            p.note = -1;
        }
        else
        {
            p.samplesLeft -= numSamples;
        }
    }
}

void GhostEngine::process (juce::MidiBuffer& midi, int numSamples, const Settings& settings, const HostTime& host)
{
    output.clear();

    // 1) Passa o MIDI original e regista as notas seguradas
    for (const auto metadata : midi)
    {
        const auto message = metadata.getMessage();

        if (message.isNoteOn())
        {
            if (heldVelocity[(size_t) message.getNoteNumber()] <= 0.0f)
                ++numHeld;
            heldVelocity[(size_t) message.getNoteNumber()] = message.getFloatVelocity();
        }
        else if (message.isNoteOff())
        {
            if (heldVelocity[(size_t) message.getNoteNumber()] > 0.0f)
                --numHeld;
            heldVelocity[(size_t) message.getNoteNumber()] = 0.0f;
        }
        else if (message.isAllNotesOff() || message.isAllSoundOff())
        {
            heldVelocity.fill (0.0f);
            numHeld = 0;
        }

        output.addEvent (message, metadata.samplePosition);
    }

    // 2) Gera os passos
    if (settings.enabled && numHeld > 0 && numSamples > 0)
    {
        const double bpm = juce::jlimit (20.0, 400.0, host.bpm);
        const double beatsPerSample = bpm / 60.0 / sampleRate;
        const double step = juce::jmax (0.05, (double) settings.beatsPerStep);

        // Com o DAW a tocar seguimos a grelha dele; parado, usamos um relógio próprio
        const double startBeats = (host.isPlaying && host.hasPosition) ? host.ppqPosition : freeRunBeats;

        for (int i = 0; i < numSamples; ++i)
        {
            const double beats = startBeats + beatsPerSample * (double) i;
            const int stepIndex = (int) std::floor (beats / step + 1.0e-9);

            if (stepIndex != lastStepIndex)
            {
                if (lastStepIndex != -1)
                    triggerStep (output, i, settings);
                lastStepIndex = stepIndex;
            }
        }

        freeRunBeats = startBeats + beatsPerSample * (double) numSamples;
    }
    else
    {
        // Sem notas seguradas: o próximo toque começa logo no passo seguinte
        lastStepIndex = -1;
        walkNote = -1;
        arpCounter = 0;
        freeRunBeats = 0.0;
    }

    // 3) Desliga as notas fantasma que já acabaram
    flushNoteOffs (output, numSamples);

    midi.swapWith (output);
}
