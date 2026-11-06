# Mugen Space Drum

Instrumento virtual VST3 inspirado nos Space Drums e handpans: um som etéreo, hipnótico e suave, feito por síntese modal, com delay e reverb atmosféricos.

**Made by Mugen** · versão 0.8.1

## Controlos

| Secção | Knob | O que faz |
|---|---|---|
| Instrument | Character | Tipo de corpo: Tongue (doce) → Handpan → Crystal (sino brilhante) |
| Instrument | Brightness | Dureza da baqueta: mais brilho metálico e mais "toque" |
| Instrument | Decay | Quanto tempo cada nota fica a soar |
| Instrument | Resonance | Batimento suave e ressonância etérea dos harmónicos |
| Sample | Hand / Felt / Shell / Air | Sample orgânico gerado pelo plugin |
| Sample | Blend | Mistura entre síntese modal e sample |
| Motion | Depth / Rate | Movimento lento e hipnótico (coro suave + esquerda/direita) |
| Delay | Mix / Time / Feedback | Ecos ping-pong entre esquerda e direita |
| Delay | Sync + divisão | Delay no tempo do projeto (1/16 a 1/1) |
| Reverb | Tipo | Room, Hall, Cathedral, Deep Space |
| Reverb | Mix / Size / Shimmer | Espaço, tamanho e brilho celestial (uma oitava acima) |
| Reverb | Freeze | Suspende a cauda no ar |
| Scale | Root / Scale / Mapping | Escala musical (41 escalas: handpan, pentatónicas, modos do Ableton, exóticas e do mundo) e forma de a tocar |
| Scale | Octave / Tune | Oitava e afinação fina |
| Ghost | Ghost On / Mode / Rate / Density / Range | Ghost: padrões generativos na escala; Arp Up / Down / Up-Down: as notas seguradas por ordem |
| Expression | Velocity / Mod Wheel / Aftertouch | Tocar suave = fofinho, forte = metal; roda e aftertouch abrem Shimmer e Motion |
| Output | Width / Volume | Largura estéreo e volume final |

## Temas

Quatro temas visuais, escolhidos no seletor da barra de topo (ficam guardados para todos os projetos):
**Mugen Nebula**, **Aurora Borealis**, **Solar Eclipse** e **Sakura Moon**.

## Presets

22 presets de fábrica (Celestial, Organic, Rhythmic, Atmosphere, Crystal, Sequences). Os presets guardados com **SAVE** ficam em
`~/Library/Audio/Presets/Mugen/Mugen Space Drum/` e aparecem na categoria **User**.

## Compilação

O plugin é compilado automaticamente pelo GitHub Actions (separador **Actions** do repositório) sempre que há alterações na branch `main`. O ficheiro final fica disponível em **Artifacts**.

Tecnologias: C++17, JUCE 8.0.14 (descarregado da fonte oficial), CMake.

## Estrutura

```
Source/      código do plugin
.github/     instruções da compilação automática
Builds/      (só local, ignorada pelo Git)
```
