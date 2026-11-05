#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>

// ==============================================================================
//  Preset Manager (Fase 6)
//
//  - Presets de fábrica, organizados por categorias (Celestial, Organic,
//    Rhythmic, Atmosphere, Crystal).
//  - Presets do utilizador: guardados como ficheiros .msdpreset em
//      ~/Library/Audio/Presets/Mugen/Mugen Space Drum/
//  - O nome do preset atual fica guardado dentro do projeto do DAW.
// ==============================================================================
class PresetManager
{
public:
    struct Entry
    {
        juce::String name;
        juce::String category;
        bool isFactory = true;
        int factoryIndex = -1;
        juce::File file;
    };

    explicit PresetManager (juce::AudioProcessorValueTreeState& state);

    /** Lista completa: fábrica primeiro, depois os presets do utilizador. */
    const std::vector<Entry>& getEntries() const noexcept { return entries; }
    static juce::StringArray categoryOrder();

    void refreshUserPresets();
    void load (int entryIndex);
    void loadNext();
    void loadPrevious();
    bool saveUserPreset (const juce::File& file);

    juce::String getCurrentName() const;
    int getCurrentIndex() const;
    int getNumFactoryPresets() const;

    static juce::File getUserPresetFolder();
    static constexpr const char* fileExtension = ".msdpreset";

private:
    void applyFactoryPreset (int factoryIndex);
    void setCurrentName (const juce::String& name);

    juce::AudioProcessorValueTreeState& parameters;
    std::vector<Entry> entries;
};
