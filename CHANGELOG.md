# Changelog — Mugen Space Drum

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
