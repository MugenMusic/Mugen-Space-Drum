#include "PresetManager.h"
#include "Parameters.h"

namespace
{
    constexpr const char* presetNameProperty = "presetName";

    // Índices úteis (ver as listas em SampleBank, ReverbEngine, DelayDivisions e ScaleEngine)
    enum Sample   { hand = 0, felt, shell, air };
    enum Reverb   { room = 0, hall, cathedral, deepSpace };
    enum Division { d16 = 0, d8T, d8, d8D, d4T, d4, d4D, d2, d1 };
    enum Root     { C = 0, Cs, D, Ds, E, F, Fs, G, Gs, A, As, B };
    enum Scale    { chromatic = 0, celticMinor, kurd, integral, pygmy, hijaz, aegean, akebono, majorPenta, minorPenta, major };

    struct FactoryPreset
    {
        const char* name;
        const char* category;
        // Instrumento
        float character, brightness, decay, resonance;
        int sample; float blend;
        // Delay
        float delayMix; bool sync; int division; float feedback;
        // Reverb
        int reverb; float reverbMix, reverbSize, shimmer;
        // Motion / saída
        float motionDepth, motionRate, width;
        // Escala
        int root, scale, mapping, octave;
        float tune, volume;
    };

    //  nome                  categoria     char  bright decay reson  sample blend   dMix sync div  fb     rev        rMix  rSize shim   mDep mRate width  root scale        map oct  tune volume
    const FactoryPreset factory[] = {
        { "Celestial Flow",   "Celestial",  0.50f, 0.45f, 5.0f, 0.70f, shell, 0.35f, 0.25f, true, d8D, 0.45f, deepSpace, 0.45f, 0.75f, 0.45f, 0.35f, 0.12f, 1.30f, D, celticMinor, 0, 0, 0.0f, -6.0f },
        { "Aurora Veil",      "Celestial",  0.85f, 0.60f, 7.0f, 0.80f, air,   0.50f, 0.20f, true, d4,  0.45f, cathedral, 0.50f, 0.80f, 0.60f, 0.50f, 0.08f, 1.60f, E, aegean,      0, 0, 0.0f, -7.0f },
        { "Starlight Bells",  "Celestial",  1.00f, 0.70f, 6.0f, 0.60f, felt,  0.20f, 0.30f, true, d8,  0.40f, hall,      0.40f, 0.80f, 0.30f, 0.25f, 0.15f, 1.30f, C, majorPenta,  0, 0, 0.0f, -7.0f },
        { "Nebula Drift",     "Celestial",  0.60f, 0.30f, 9.0f, 0.90f, air,   0.65f, 0.30f, true, d4D, 0.60f, deepSpace, 0.60f, 0.95f, 0.70f, 0.60f, 0.05f, 1.80f, A, kurd,        0, 0, 0.0f, -7.0f },

        { "Warm Handpan",     "Organic",    0.50f, 0.40f, 4.5f, 0.50f, shell, 0.40f, 0.12f, true, d8,  0.30f, hall,      0.30f, 0.60f, 0.00f, 0.15f, 0.15f, 1.10f, D, celticMinor, 0, 0, 0.0f, -6.0f },
        { "Hand Touch",       "Organic",    0.45f, 0.35f, 3.0f, 0.40f, hand,  0.55f, 0.10f, true, d8,  0.25f, room,      0.25f, 0.50f, 0.00f, 0.10f, 0.20f, 1.00f, D, kurd,        0, 0, 0.0f, -6.0f },
        { "Felt Lullaby",     "Organic",    0.20f, 0.25f, 5.0f, 0.50f, felt,  0.50f, 0.20f, true, d4,  0.35f, hall,      0.40f, 0.70f, 0.15f, 0.25f, 0.10f, 1.20f, C, majorPenta,  0, 0, 0.0f, -6.0f },
        { "Tongue Drum Garden","Organic",   0.00f, 0.40f, 3.5f, 0.35f, felt,  0.30f, 0.18f, true, d8D, 0.35f, room,      0.30f, 0.60f, 0.00f, 0.15f, 0.20f, 1.20f, G, pygmy,       0, 0, 0.0f, -6.0f },

        { "Pulse Temple",     "Rhythmic",   0.50f, 0.55f, 2.0f, 0.30f, hand,  0.40f, 0.35f, true, d16, 0.40f, room,      0.20f, 0.40f, 0.00f, 0.10f, 0.25f, 1.10f, D, hijaz,       0, 0, 0.0f, -6.0f },
        { "Echo Steps",       "Rhythmic",   0.55f, 0.50f, 3.0f, 0.50f, shell, 0.30f, 0.45f, true, d8D, 0.55f, hall,      0.30f, 0.60f, 0.10f, 0.15f, 0.20f, 1.30f, E, minorPenta,  0, 0, 0.0f, -7.0f },
        { "Desert Caravan",   "Rhythmic",   0.60f, 0.50f, 3.5f, 0.55f, hand,  0.35f, 0.30f, true, d8T, 0.40f, cathedral, 0.30f, 0.60f, 0.10f, 0.20f, 0.18f, 1.30f, E, hijaz,       0, 0, 0.0f, -6.0f },

        { "Frozen Cathedral", "Atmosphere", 0.70f, 0.40f, 10.0f,0.85f, air,   0.40f, 0.20f, true, d2,  0.50f, cathedral, 0.60f, 1.00f, 0.50f, 0.40f, 0.06f, 1.50f, D, celticMinor, 0, 0, 0.0f, -7.0f },
        { "Deep Space Choir", "Atmosphere", 0.75f, 0.35f, 12.0f,1.00f, air,   0.75f, 0.15f, true, d1,  0.50f, deepSpace, 0.70f, 1.00f, 0.80f, 0.50f, 0.04f, 2.00f, C, aegean,      0,-1, 0.0f, -7.0f },
        { "Ocean Floor",      "Atmosphere", 0.30f, 0.20f, 8.0f, 0.70f, air,   0.50f, 0.25f, true, d4D, 0.55f, deepSpace, 0.55f, 0.85f, 0.20f, 0.45f, 0.07f, 1.60f, A, kurd,        0,-1, 0.0f, -6.0f },

        { "Crystal Garden",   "Crystal",    0.95f, 0.65f, 5.0f, 0.55f, felt,  0.20f, 0.30f, true, d8,  0.45f, hall,      0.40f, 0.75f, 0.35f, 0.30f, 0.20f, 1.40f, G, major,       0, 0, 0.0f, -7.0f },
        { "Glass Moon",       "Crystal",    1.00f, 0.50f, 8.0f, 0.75f, air,   0.30f, 0.25f, true, d4D, 0.50f, cathedral, 0.50f, 0.90f, 0.50f, 0.35f, 0.09f, 1.50f, B, minorPenta,  0, 0, 0.0f, -7.0f },
    };

    constexpr int numFactory = (int) (sizeof (factory) / sizeof (factory[0]));
}

// ------------------------------------------------------------------------------
PresetManager::PresetManager (juce::AudioProcessorValueTreeState& state)
    : parameters (state)
{
    refreshUserPresets();
}

juce::StringArray PresetManager::categoryOrder()
{
    return { "Celestial", "Organic", "Rhythmic", "Atmosphere", "Crystal", "User" };
}

juce::File PresetManager::getUserPresetFolder()
{
    return juce::File::getSpecialLocation (juce::File::userHomeDirectory)
               .getChildFile ("Library/Audio/Presets/Mugen/Mugen Space Drum");
}

int PresetManager::getNumFactoryPresets() const { return numFactory; }

void PresetManager::refreshUserPresets()
{
    entries.clear();

    for (int i = 0; i < numFactory; ++i)
        entries.push_back ({ factory[i].name, factory[i].category, true, i, {} });

    const auto folder = getUserPresetFolder();
    if (folder.isDirectory())
    {
        auto files = folder.findChildFiles (juce::File::findFiles, false, juce::String ("*") + fileExtension);
        files.sort();
        for (const auto& f : files)
            entries.push_back ({ f.getFileNameWithoutExtension(), "User", false, -1, f });
    }
}

juce::String PresetManager::getCurrentName() const
{
    return parameters.state.getProperty (presetNameProperty, "Custom").toString();
}

void PresetManager::setCurrentName (const juce::String& name)
{
    parameters.state.setProperty (presetNameProperty, name, nullptr);
}

int PresetManager::getCurrentIndex() const
{
    const auto name = getCurrentName();
    for (size_t i = 0; i < entries.size(); ++i)
        if (entries[i].name == name)
            return (int) i;
    return -1;
}

void PresetManager::applyFactoryPreset (int index)
{
    const auto& p = factory[juce::jlimit (0, numFactory - 1, index)];

    auto set = [this] (const char* id, float value)
    {
        if (auto* param = parameters.getParameter (id))
        {
            param->beginChangeGesture();
            param->setValueNotifyingHost (param->convertTo0to1 (value));
            param->endChangeGesture();
        }
    };

    set (ParamIDs::character,     p.character);
    set (ParamIDs::brightness,    p.brightness);
    set (ParamIDs::decay,         p.decay);
    set (ParamIDs::resonance,     p.resonance);
    set (ParamIDs::sampleType,    (float) p.sample);
    set (ParamIDs::blend,         p.blend);

    set (ParamIDs::delayMix,      p.delayMix);
    set (ParamIDs::delaySync,     p.sync ? 1.0f : 0.0f);
    set (ParamIDs::delayDivision, (float) p.division);
    set (ParamIDs::delayFeedback, p.feedback);

    set (ParamIDs::reverbType,    (float) p.reverb);
    set (ParamIDs::reverbMix,     p.reverbMix);
    set (ParamIDs::reverbSize,    p.reverbSize);
    set (ParamIDs::shimmer,       p.shimmer);
    set (ParamIDs::freeze,        0.0f);

    set (ParamIDs::motionDepth,   p.motionDepth);
    set (ParamIDs::motionRate,    p.motionRate);
    set (ParamIDs::width,         p.width);

    set (ParamIDs::scaleRoot,     (float) p.root);
    set (ParamIDs::scaleType,     (float) p.scale);
    set (ParamIDs::scaleMapping,  (float) p.mapping);
    set (ParamIDs::octave,        (float) p.octave);
    set (ParamIDs::fineTune,      p.tune);
    set (ParamIDs::volume,        p.volume);
}

void PresetManager::load (int entryIndex)
{
    if (entryIndex < 0 || entryIndex >= (int) entries.size())
        return;

    const auto entry = entries[(size_t) entryIndex];

    if (entry.isFactory)
    {
        applyFactoryPreset (entry.factoryIndex);
    }
    else
    {
        auto xml = juce::XmlDocument::parse (entry.file);
        if (xml == nullptr || ! xml->hasTagName (parameters.state.getType()))
            return;

        parameters.replaceState (juce::ValueTree::fromXml (*xml));
    }

    setCurrentName (entry.name);
}

void PresetManager::loadNext()
{
    if (entries.empty())
        return;
    const int current = getCurrentIndex();
    load ((current + 1) % (int) entries.size());
}

void PresetManager::loadPrevious()
{
    if (entries.empty())
        return;
    const int current = getCurrentIndex();
    load (current <= 0 ? (int) entries.size() - 1 : current - 1);
}

bool PresetManager::saveUserPreset (const juce::File& requestedFile)
{
    auto file = requestedFile.withFileExtension (fileExtension);
    file.getParentDirectory().createDirectory();

    setCurrentName (file.getFileNameWithoutExtension());

    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    if (xml == nullptr || ! xml->writeTo (file))
        return false;

    refreshUserPresets();
    return true;
}
