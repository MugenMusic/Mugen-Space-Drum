#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "MugenLookAndFeel.h"
#include "OrbVisualizer.h"

// ==============================================================================
//  PluginEditor — a janela do plugin (Fase 6)
//
//  Inspirada na organização de um sintetizador profissional:
//   - Cabeçalho: logótipo, barra de presets (◀ nome ▶ SAVE), luz MIDI.
//   - Separadores: INSTRUMENT · RESONANCE · AMBIENCE · MOTION · SCALE & TUNING.
//   - Palco central: controlos à esquerda e à direita, orbe animado no meio.
//   - Browser de presets à direita, organizado por categorias.
//   - Teclado em baixo, com as notas da escala marcadas.
//
//  A janela pode ser redimensionada (arrastar o canto inferior direito).
// ==============================================================================
class MugenSpaceDrumEditor : public juce::AudioProcessorEditor
{
public:
    explicit MugenSpaceDrumEditor (MugenSpaceDrumProcessor&);
    ~MugenSpaceDrumEditor() override;

    void resized() override;

    static constexpr int designWidth  = 1180;
    static constexpr int designHeight = 760;

private:
    class MainView;
    MugenLookAndFeel lookAndFeel;          // declarado antes da vista: é destruído depois dela
    std::unique_ptr<MainView> view;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MugenSpaceDrumEditor)
};
