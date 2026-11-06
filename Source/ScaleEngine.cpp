#include "ScaleEngine.h"

const std::vector<ScaleEngine::Scale>& ScaleEngine::scales()
{
    // Escalas típicas de handpan e escalas pentatónicas.
    static const std::vector<Scale> list {
        // Atenção: só acrescentar no fim — os presets e os projetos guardam o número da escala.
        { "Chromatic",          { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 } },
        // Handpan
        { "Celtic Minor",       { 0, 2, 3, 5, 7, 10 } },
        { "Kurd",               { 0, 2, 3, 5, 7, 8, 10 } },
        { "Integral",           { 0, 2, 3, 7, 8, 10 } },
        { "Pygmy",              { 0, 2, 3, 7, 10 } },
        { "Hijaz",              { 0, 1, 4, 5, 7, 8, 10 } },
        { "Aegean",             { 0, 4, 6, 7, 11 } },
        { "Akebono",            { 0, 2, 3, 7, 8 } },
        // Pentatónicas
        { "Major Pentatonic",   { 0, 2, 4, 7, 9 } },
        { "Minor Pentatonic",   { 0, 3, 5, 7, 10 } },
        // Maior e modos (como no Ableton Live)
        { "Major",              { 0, 2, 4, 5, 7, 9, 11 } },
        { "Minor",              { 0, 2, 3, 5, 7, 8, 10 } },
        { "Dorian",             { 0, 2, 3, 5, 7, 9, 10 } },
        { "Phrygian",           { 0, 1, 3, 5, 7, 8, 10 } },
        { "Lydian",             { 0, 2, 4, 6, 7, 9, 11 } },
        { "Mixolydian",         { 0, 2, 4, 5, 7, 9, 10 } },
        { "Locrian",            { 0, 1, 3, 5, 6, 8, 10 } },
        // Harmónicas, melódicas e exóticas
        { "Harmonic Minor",     { 0, 2, 3, 5, 7, 8, 11 } },
        { "Melodic Minor",      { 0, 2, 3, 5, 7, 9, 11 } },
        { "Harmonic Major",     { 0, 2, 4, 5, 7, 8, 11 } },
        { "Phrygian Dominant",  { 0, 1, 4, 5, 7, 8, 10 } },
        { "Dorian #4",          { 0, 2, 3, 6, 7, 9, 10 } },
        { "Lydian Dominant",    { 0, 2, 4, 6, 7, 9, 10 } },
        { "Lydian Augmented",   { 0, 2, 4, 6, 8, 9, 11 } },
        { "Super Locrian",      { 0, 1, 3, 4, 6, 8, 10 } },
        { "Hungarian Minor",    { 0, 2, 3, 6, 7, 8, 11 } },
        { "Bhairav",            { 0, 1, 4, 5, 7, 8, 11 } },
        // Simétricas e blues
        { "Whole Tone",         { 0, 2, 4, 6, 8, 10 } },
        { "Half-Whole Dim.",    { 0, 1, 3, 4, 6, 7, 9, 10 } },
        { "Whole-Half Dim.",    { 0, 2, 3, 5, 6, 8, 9, 11 } },
        { "Minor Blues",        { 0, 3, 5, 6, 7, 10 } },
        { "Major Blues",        { 0, 2, 3, 4, 7, 9 } },
        { "8-Tone Spanish",     { 0, 1, 3, 4, 5, 6, 8, 10 } },
        // Do mundo
        { "Hirajoshi",          { 0, 2, 3, 7, 8 } },
        { "In-Sen",             { 0, 1, 5, 7, 10 } },
        { "Iwato",              { 0, 1, 5, 6, 10 } },
        { "Kumoi",              { 0, 2, 3, 7, 9 } },
        { "Pelog Selisir",      { 0, 1, 3, 7, 8 } },
        { "Pelog Tembung",      { 0, 1, 5, 7, 8 } },
        { "Egyptian",           { 0, 2, 5, 7, 10 } },
        { "Oxalis",             { 0, 2, 3, 5, 7, 9 } }
    };
    return list;
}

const std::vector<std::pair<int, const char*>>& ScaleEngine::scaleGroups()
{
    // Títulos do menu das escalas: aparecem antes da escala com este número.
    static const std::vector<std::pair<int, const char*>> groups {
        { 0,  "BASIC" },
        { 1,  "HANDPAN" },
        { 8,  "PENTATONIC" },
        { 10, "MAJOR & MODES" },
        { 17, "HARMONIC & EXOTIC" },
        { 27, "SYMMETRIC & BLUES" },
        { 33, "WORLD" }
    };
    return groups;
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
