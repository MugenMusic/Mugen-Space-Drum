# Changelog — Mugen Space Drum

## v0.8.2 — Correções e equilíbrio

- **Logótipo central**: passa a dizer **SINGULARITY** (3D, com espaçamento ajustado a palavras longas);
  a arte de cada tema (planeta, sol, lua) foi afastada para a direita para lhe dar espaço.
- **Corrigido o "pitch estranho" ao mudar de preset**: o delay e o reverb já não esticam as linhas de atraso
  (efeito de fita a mudar de velocidade, que chegava a subir os ecos mais de duas oitavas).
  Em saltos grandes (outra divisão do delay, outro tipo de sala, tamanho muito diferente) o efeito desvanece,
  muda em silêncio e volta. Pequenos ajustes nos knobs continuam suaves.
- **Presets etéreos equilibrados**: o reverb Deep Space passa de 8–25 s para 5–14 s de cauda; os presets
  Celestial Flow, Nebula Drift, Deep Space Choir, Ocean Floor, Frozen Cathedral, Aurora Veil, Glass Moon,
  Dream Sequencer, Starfall e Lunar Steps têm menos reverb/shimmer/feedback (todas as caudas descem 40 dB
  entre 8 e 12 s, em vez de ficarem a soar mais de 16 s).
- **Volumes dos presets igualados** (antes havia ~15 dB de diferença; agora todos dentro de ±0,5 dB).
- **Sem cliques com muitas notas**: 32 vozes (antes 16) e anti-clique quando uma voz é reutilizada ou cortada
  pelo DAW (teste com Ghost Notes a 1/16 e caudas de 10 s: de 52 cliques para 0).

## v0.8.1 — Acabamento premium

- **Logótipo MUGEN 3D**: letras esculpidas com extrusão em 16 camadas (profundidade real), face cromada
  com horizonte refletido, bisel de luz, sombra projetada, aura suave, um brilho que atravessa as letras
  a cada ~7 segundos e cintilações nas pontas. Muda de cor com cada tema.
- **Emblemas de preset**: no centro do orbe, cada preset tem o seu emblema de geometria sagrada, próprio
  da categoria e único para cada preset (orientação e número de pétalas/pontas variam):
  Celestial (estrela {n/k} com órbitas e planetas), Organic (lótus de duas camadas), Rhythmic (pele de
  tambor que pulsa, hexagrama), Atmosphere (flor da vida e anel de vento), Crystal (hexágonos com
  facetas e cintilações), Sequences (espiral de ouro com onda a viajar), User (mandala).
- **Transição etérea** ao mudar de preset: o emblema antigo dissolve-se a expandir, o novo desenha-se
  do centro, com um clarão suave.
- **Orbe novo**: esfera de vidro com aro de luz (Fresnel), reflexos e cáustica; raios de luz volumétricos
  a rodar devagar; poeira de luz a subir (mais quando há som); bloom em todos os traços.

## v0.8.0 — Arp, Sequences e temas

- **Modo Arp** nas Ghost Notes: novo seletor *Mode* com **Ghost** (generativo), **Arp Up**, **Arp Down** e **Arp Up/Down**.
  O arpejo toca as notas seguradas por ordem, repetidas por *Range* oitavas, no tempo do DAW; *Density* cria pausas.
- **6 presets novos** na categoria **Sequences**, já com Ghost Notes ou Arp ligados:
  Dream Sequencer, Zen Garden Walk (Hirajoshi), Starfall (Aegean), Phrygian Caravan, Crystal Arpeggio, Lunar Steps.
  Os presets passam a definir também as Ghost Notes (os presets antigos desligam-nas).
- **3 temas novos**, desenhados de raiz, com arte própria no cabeçalho:
  **Aurora Borealis** (cortinas de aurora esmeralda e ciano), **Solar Eclipse** (sol negro com coroa dourada e brasas)
  e **Sakura Moon** (lua cheia e pétalas de cerejeira). O original chama-se agora **Mugen Nebula** (com planeta).
  Seletor de tema na barra de topo; a escolha fica guardada no Mac e vale para todos os projetos.
  As estrelas cadentes usam as cores de cada tema.

## v0.7.1 — Mais escalas e chuva de estrelas

- **41 escalas** (antes 11), num menu organizado por famílias:
  Basic · Handpan · Pentatonic · Major & Modes (Minor, Dorian, Phrygian, Lydian, Mixolydian, Locrian) ·
  Harmonic & Exotic (Harmonic/Melodic Minor, Harmonic Major, Phrygian Dominant, Dorian #4, Lydian Dominant,
  Lydian Augmented, Super Locrian, Hungarian Minor, Bhairav) · Symmetric & Blues (Whole Tone, Half-Whole Dim.,
  Whole-Half Dim., Minor/Major Blues, 8-Tone Spanish) · World (Hirajoshi, In-Sen, Iwato, Kumoi, Pelog Selisir,
  Pelog Tembung, Egyptian, Oxalis). As escalas antigas mantêm o número: presets e projetos não mudam.
- **Estrelas cadentes**: muitas mais (~1 por segundo e pequenas chuvas de vez em quando), coloridas —
  brancas, azul e violeta Mugen, turquesa de aurora, rosa nebulosa e, raras, douradas.
- Acertos: pastilha GHOST já não tapa o EXPR; nomes de presets longos cabem inteiros no carrossel;
  pontas do carrossel desvanecem; etiquetas ajustam-se a escalas com nomes longos.

## v0.7.0 — Expressão, Ghost Notes e estrelas cadentes

- **Velocidade e expressão**: novo knob *Velocity* (sensibilidade). Tocar suave dá um som abafado e "fofinho"
  (baqueta macia, ataque redondo); tocar forte dá brilho e metal. A 0 % todas as notas soam iguais.
- **Roda de modulação (CC1) e aftertouch** (de canal e polifónico) abrem o **Shimmer** e o **Motion** enquanto tocas.
  Knobs *Mod Wheel* e *Aftertouch* definem quanto. Barra **EXPR** no palco mostra a expressão em tempo real.
- **Ghost Notes**: seguras uma ou mais notas e o plugin toca sozinho padrões generativos dentro da escala escolhida,
  sincronizados com o BPM e a grelha do Ableton (ou com relógio próprio com o DAW parado).
  *Rate* (1/4, 1/8T, 1/8, 1/16T, 1/16), *Density* (pausas) e *Range* (1 a 3 oitavas). Indicador **GHOST** no palco.
- Novo separador **GHOST & EXPRESSION**; separadores ocupam agora toda a largura.
- **Estrelas cadentes**: passam pelo fundo e atravessam os painéis em transparência suave; campo de estrelas no corpo
  da janela; botão **STARS** na barra de topo para ligar/desligar (fica guardado no projeto).

## v0.6.2 — Palco central dinâmico

- **Palco central** maior, com ecrã técnico (grelha subtil, nebulosa, indicador LIVE ENGINE, contador PRESET 03/16).
- **Carrossel de presets animado**: o preset atual em destaque e os vizinhos a desvanecer dos lados;
  desliza ao mudar de preset (pelo browser, pelas setas, pelo menu ou pelo DAW).
  Clicar nos lados ou rodar a roda do rato muda de preset.
- **Etiquetas do som** por baixo: sample, corpo, tipo de reverb e escala — atualizam ao vivo.
- **Anel de onda ao vivo** à volta do orbe: desenha o som real que sai do plugin, com mostrador em marcas finas.
- Painéis com gradiente e fio de luz no topo; macros com valor em percentagem.
- Octave e Tune passam para o separador Scale & Tuning; faixa OUT com fader de volume e Width.

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
