# Changelog — Mugen Space Drum

## v0.6.1 — Nova interface (estilo sintetizador profissional)

- Organização inspirada nos grandes sintetizadores, com visual Mugen moderno:
  browser de **PATCHES** à esquerda (categoria, pesquisa, capa gerada por categoria, lista, ◀ ▶, RANDOM, SAVE),
  faixa de cabeçalho com logótipo e ecrã do preset, separadores, painéis com cabeçalho,
  **ecrã central com o orbe**, faixas **TUNING** (Octave, Tune) e **OUTPUT** (fader de volume, Width),
  **macros** sempre visíveis (Character, Blend, Space, Motion) e teclado.
- Knobs novos: anel luminoso, corpo com luz e reflexo, ponteiro em linha.
- Faders em cápsula, seletores planos com seta, botões arredondados.
- Novos visuais: **Scale Map** (círculo das 12 notas com a escala desenhada) e **Stereo Field** (movimento L/R animado).
- Correções: valores dos knobs sem moldura branca; nome de cada knob logo por baixo do knob.
- A janela abre a 85 % e pode ser redimensionada (60 % a 130 %).

## v0.6.0 — Fase 6: Presets & UI

- **Nova interface** ao estilo de um sintetizador profissional, com a identidade Mugen:
  cabeçalho com barra de presets, 5 separadores (Instrument · Resonance · Ambience · Motion · Scale & Tuning),
  palco central com o **orbe animado** e **browser de presets** à direita.
- **Orbe animado**: anéis de luz, flor de lótus, partículas e onda de energia; pulsa a cada nota,
  brilha com o som, a cor segue o Character e a rotação segue o Motion Rate.
- **16 presets de fábrica** em 5 categorias: Celestial, Organic, Rhythmic, Atmosphere, Crystal
  (inclui "Celestial Flow", o preset inicial).
- **Presets do utilizador**: botão SAVE (ficheiros .msdpreset em ~/Library/Audio/Presets/Mugen/Mugen Space Drum).
- Navegação com ◀ ▶, menu por categorias ao clicar no nome, e lista no browser.
- O nome do preset e o separador aberto ficam guardados no projeto do DAW.
- Os presets de fábrica aparecem também como "programas" no DAW.
- **Janela redimensionável** (60 % a 150 %), sempre com as mesmas proporções.

## v0.5.0 — Fase 5: Musical System

- Novo **Scale Engine**: as notas tocadas passam a pertencer sempre à escala escolhida.
- 11 escalas: Chromatic (desligado), Celtic Minor, Kurd, Integral, Pygmy, Hijaz, Aegean, Akebono, Major Pentatonic, Minor Pentatonic, Major.
- **Root**: nota raiz de C a B (por defeito D, como muitos handpans).
- **Mapping**: *Nearest* (cada tecla toca a nota da escala mais próxima) ou *White Keys* (as teclas brancas tocam a escala nota a nota, Dó central = raiz).
- **Octave** (−2 a +2) e **Tune** (afinação fina ±50 cêntimos; ex. −32 ct ≈ A=432 Hz).
- O teclado no ecrã marca as notas da escala (violeta) e a nota raiz (azul).
- O Note Off desliga sempre a nota certa, mesmo que a escala mude com uma tecla premida.

## v0.4.0 — Fase 4: Ambience & Motion

- **Novo reverb próprio** (rede de 8 linhas com difusão, modulação e amortecimento) com 4 tipos: **Room, Hall, Cathedral, Deep Space** (até ~25 s de cauda).
- **Shimmer**: a cauda sobe uma oitava e volta a entrar no reverb (protegido para nunca descontrolar).
- **Freeze**: a cauda fica suspensa no ar enquanto o botão estiver ligado.
- **Delay sincronizado** com o BPM do DAW (botão SYNC + divisões 1/16 a 1/1, incluindo pontuadas e tercinas). Delay até 4 s.
- Nova secção **MOTION**: Depth e Rate (coro suave + movimento lento esquerda/direita).
- **Width**: largura estéreo de 0 % a 200 %.
- Interface reorganizada em duas filas: INSTRUMENT · SAMPLE · MOTION · OUTPUT / DELAY · REVERB.

## v0.3.0 — Fase 3: Hybrid Engine

- Novo **Sample Engine**: samples orgânicos gerados pelo próprio plugin (sem gravações de terceiros).
- Quatro samples: **Hand** (mão), **Felt** (feltro), **Shell** (casca metálica rica), **Air** (sopro etéreo).
- Cada sample é "gravado" em 5 notas de referência (C1 a C5) e afinado para cada nota.
- Novo knob **Blend**: mistura síntese modal ↔ sample (volume constante ao rodar).
- Nova secção **SAMPLE** na interface, com seletor de sample.
- A descida de afinação do ataque também se aplica ao sample.

## v0.2.0 — Fase 2: Modal Synthesis

- Motor modal com 10 modos de vibração por nota (antes 6).
- Novo knob **Character**: morph contínuo Tongue → Handpan → Crystal.
- Modelo de baqueta: o Brightness define a dureza da baqueta e o tempo de contacto com o metal.
- Ligeira descida de afinação no ataque, como num handpan real.
- Pequenas variações orgânicas em cada toque (duas notas iguais nunca soam iguais).
- Secção TONE passa a chamar-se INSTRUMENT (Character, Brightness, Decay, Resonance).
- Compilação no GitHub lê a versão automaticamente.

## v0.1.0 — primeira versão

- Instrumento VST3 (MIDI → áudio estéreo), macOS Apple Silicon.
- Motor de síntese modal: 6 parciais com afinação de tambor metálico, toque suave de baqueta.
- Até 16 notas em simultâneo; as notas continuam a ressoar depois de largar a tecla.
- Knobs: Brightness, Decay, Resonance · Delay (Mix, Time, Feedback) · Reverb (Mix, Size) · Volume.
- Delay ping-pong quente e reverb atmosférico.
- Interface escura violeta/azul, luz MIDI, teclado no ecrã, "Made by Mugen".
- Os valores dos knobs ficam guardados no projeto do DAW.
- Compilação automática no GitHub Actions.
