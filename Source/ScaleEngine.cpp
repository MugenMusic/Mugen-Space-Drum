#include "ScaleEngine.h"

const std::vector<ScaleEngine::Scale>& ScaleEngine::scales()
{
    // Escalas típicas de handpan e escalas pentatónicas.
    static const std::vector<Scale> list {
        { "Chromatic",        { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 } },
        { "Celtic Minor",     { 0, 2, 3, 5, 7, 10 } },
        { "Kurd",             { 0, 2, 3, 5, 7, 8, 10 } },
        { "Integral",         { 0, 2, 3, 7, 8, 10 } },
        { "Pygmy",            { 0, 2, 3, 7, 10 } },
        { "Hijaz",            { 0, 1, 4, 5, 7, 8, 10 } },
        { "Aegean",           { 0, 4, 6, 7, 11 } },
        { "Akebono",          { 0, 2, 3, 7, 8 } },
        { "Major Pentatonic", { 0, 2, 4, 7, 9 } },
        { "Minor Pentatonic", { 0, 3, 5, 7, 10 } },
        { "Major",            { 0, 2, 4, 5, 7, 9, 11 } }
    };
    return list;
}

juce::StringArray ScaleEngine::scaleNames()
{
    juce::StringArray names;
    for (const auto& s : scales())
        names.add (s.name);
    return names;
}

namespace
{
    int floorDiv (int a, int b) noexcept { return (a >= 0) ? a / b : -((-a + b - 1) / b); }
    int floorMod (int a, int b) noexcept { return a - floorDiv (a, b) * b; }

    // Posição de cada nota dentro das 7 teclas brancas de uma oitava (-1 = tecla preta)
    constexpr std::array<int, 12> whiteKeyIndex { 0, -1, 1, -1, 2, 3, -1, 4, -1, 5, -1, 6 };
}

bool ScaleEngine::isInScale (int midiNote, int root, int scaleIndex) noexcept
{
    const auto& list = scales();
    const auto& scale = list[(size_t) juce::jlimit (0, (int) list.size() - 1, scaleIndex)];
    const int relative = floorMod (midiNote - root, 12);

    for (int interval : scale.intervals)
        if (interval == relative)
            return true;

    return false;
}

int ScaleEngine::mapNote (int midiNote, int root, int scaleIndex, int mapping, int octave) noexcept
{
    const auto& list = scales();
    const auto& intervals = list[(size_t) juce::jlimit (0, (int) list.size() - 1, scaleIndex)].intervals;
    root = juce::jlimit (0, 11, root);

    int result = midiNote;
    const int pitchClass = floorMod (midiNote, 12);

    if (mapping == whiteKeys && whiteKeyIndex[(size_t) pitchClass] >= 0)
    {
        // Teclas brancas: cada uma é o grau seguinte da escala. Dó central = nota raiz.
        const int whiteNumber = floorDiv (midiNote, 12) * 7 + whiteKeyIndex[(size_t) pitchClass];
        const int degree = whiteNumber - 35;   // 35 = Dó central (nota 60)
        const int size = (int) intervals.size();
        const int baseNote = 60 + (root <= 6 ? root : root - 12);

        result = baseNote + 12 * floorDiv (degree, size) + intervals[(size_t) floorMod (degree, size)];
    }
    else
    {
        // Nota da escala mais próxima (em caso de empate, a de baixo).
        const int relative = floorMod (midiNote - root, 12);
        int best = midiNote;
        int bestDistance = 1000;

        for (int interval : intervals)
        {
            for (int shift = -12; shift <= 12; shift += 12)
            {
                const int candidate = midiNote - relative + interval + shift;
                const int distance = std::abs (candidate - midiNote);
                if (distance < bestDistance || (distance == bestDistance && candidate < best))
                {
                    best = candidate;
                    bestDistance = distance;
                }
            }
        }
        result = best;
    }

    result += 12 * juce::jlimit (-2, 2, octave);
    return juce::jlimit (0, 127, result);
}

void ScaleEngine::prepare()
{
    output.ensureSize (8192);   // espaço reservado: sem alocações durante o áudio
    reset();
}

void ScaleEngine::reset()
{
    for (auto& channel : activeNotes)
        channel.fill (-1);
}

void ScaleEngine::process (juce::MidiBuffer& midi, int root, int scaleIndex, int mapping, int octave)
{
    if (midi.isEmpty())
        return;

    output.clear();

    for (const auto metadata : midi)
    {
        auto message = metadata.getMessage();

        if (message.isNoteOn() || message.isNoteOff())
        {
            const int channel = juce::jlimit (1, 16, message.getChannel()) - 1;
            const int input = message.getNoteNumber();
            auto& stored = activeNotes[(size_t) channel][(size_t) input];

            if (message.isNoteOn())
            {
                stored = mapNote (input, root, scaleIndex, mapping, octave);
                message.setNoteNumber (stored);
            }
            else
            {
                message.setNoteNumber (stored >= 0 ? stored : mapNote (input, root, scaleIndex, mapping, octave));
                stored = -1;
            }
        }
        else if (message.isAllNotesOff() || message.isAllSoundOff())
        {
            reset();
        }

        output.addEvent (message, metadata.samplePosition);
    }

    midi.swapWith (output);
}
