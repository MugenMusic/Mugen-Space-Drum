#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "MugenLookAndFeel.h"
#include "OrbVisualizer.h"

// ==============================================================================
//  PluginEditor — a janela do plugin (v0.6.1)
//
//  Organização inspirada nos sintetizadores profissionais, com visual Mugen:
//   - À esquerda: browser de PATCHES (categoria, pesquisa, capa, lista, ferramentas).
//   - Faixa de cabeçalho: logótipo e ecrã do preset (◀ ▶, menu ao clicar).
//   - Separadores: INSTRUMENT · RESONANCE · AMBIENCE · MOTION · SCALE & TUNING.
//   - Corpo: faixa TUNING | painel | ecrã com o orbe | painel | faixa OUTPUT.
//   - Macros sempre visíveis e teclado com a escala marcada.
//
//  A janela pode ser redimensionada (arrastar o canto inferior direito).
// ==============================================================================
class MugenSpaceDrumEditor : public juce::AudioProcessorEditor
{
public:
    explicit MugenSpaceDrumEditor (MugenSpaceDrumProcessor&);
    ~MugenSpaceDrumEditor() override;

    void resized() override;

    static constexpr int designWidth  = 1280;
    static constexpr int designHeight = 800;

private:
    class MainView;
    MugenLookAndFeel lookAndFeel;          // declarado antes da vista: é destruído depois dela
    std::unique_ptr<MainView> view;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MugenSpaceDrumEditor)
};
