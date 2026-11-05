# Mugen Space Drum

Instrumento virtual VST3 inspirado nos Space Drums e handpans: um som etéreo, hipnótico e suave, feito por síntese modal, com delay e reverb atmosféricos.

**Made by Mugen** · versão 0.1.0

## Controlos

| Secção | Knob | O que faz |
|---|---|---|
| Tone | Brightness | Mais brilho metálico e mais "toque" da baqueta |
| Tone | Decay | Quanto tempo cada nota fica a soar |
| Tone | Resonance | Batimento suave e ressonância etérea dos harmónicos |
| Delay | Mix / Time / Feedback | Ecos ping-pong entre esquerda e direita |
| Reverb | Mix / Size | Espaço atmosférico à volta do som |
| Output | Volume | Volume final |

## Compilação

O plugin é compilado automaticamente pelo GitHub Actions (separador **Actions** do repositório) sempre que há alterações na branch `main`. O ficheiro final fica disponível em **Artifacts**.

Tecnologias: C++17, JUCE 8.0.14 (descarregado da fonte oficial), CMake.

## Estrutura

```
Source/      código do plugin
.github/     instruções da compilação automática
Builds/      (só local, ignorada pelo Git)
```
