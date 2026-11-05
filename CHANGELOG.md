# Changelog — Mugen Space Drum

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
