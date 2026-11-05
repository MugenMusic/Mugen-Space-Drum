#pragma once

#include <array>
#include <cmath>

// ==============================================================================
//  Modal Model (Fase 2)
//
//  Descreve os "modos" de vibração do instrumento: em que frequências o metal
//  vibra (ratios), com que força (levels) e durante quanto tempo (lengths).
//
//  O knob Character faz um morph contínuo entre três corpos:
//     0.0  Tongue  – tambor de línguas: suave, quase harmónico, doce
//     0.5  Handpan – modos afinados 1 : 2 : 3 + modos inarmónicos da "casca"
//     1.0  Crystal – sino / cristal: brilhante, inarmónico, cintilante
// ==============================================================================
namespace ModalModel
{
    constexpr int numModes = 10;

    struct ModeSet
    {
        std::array<double, numModes> ratios;
        std::array<double, numModes> levels;
        std::array<double, numModes> lengths;
    };

    inline const ModeSet& tongue()
    {
        static const ModeSet set {
            {{ 1.0, 2.0, 3.0, 4.02, 5.05, 6.10, 7.30, 8.60, 10.2, 12.1 }},
            {{ 1.0, 0.45, 0.25, 0.12, 0.07, 0.045, 0.030, 0.020, 0.012, 0.008 }},
            {{ 1.0, 0.65, 0.45, 0.30, 0.22, 0.16, 0.12, 0.09, 0.07, 0.05 }}
        };
        return set;
    }

    inline const ModeSet& handpan()
    {
        static const ModeSet set {
            {{ 1.0, 2.0, 3.0, 2.40, 3.60, 4.50, 5.20, 6.40, 7.90, 9.30 }},
            {{ 1.0, 0.55, 0.35, 0.12, 0.08, 0.10, 0.06, 0.05, 0.03, 0.02 }},
            {{ 1.0, 0.80, 0.60, 0.25, 0.20, 0.30, 0.15, 0.12, 0.08, 0.06 }}
        };
        return set;
    }

    inline const ModeSet& crystal()
    {
        static const ModeSet set {
            {{ 1.0, 2.0, 2.76, 3.90, 5.40, 6.80, 8.93, 10.7, 13.3, 16.1 }},
            {{ 1.0, 0.35, 0.50, 0.20, 0.30, 0.12, 0.15, 0.07, 0.06, 0.03 }},
            {{ 1.0, 0.90, 0.85, 0.50, 0.60, 0.35, 0.40, 0.25, 0.20, 0.15 }}
        };
        return set;
    }

    /** Mistura dois conjuntos. As frequências misturam-se em escala logarítmica
        (como o ouvido as percebe); volumes e durações misturam-se linearmente. */
    inline ModeSet blend (const ModeSet& a, const ModeSet& b, double t)
    {
        ModeSet out {};
        for (size_t k = 0; k < (size_t) numModes; ++k)
        {
            out.ratios[k]  = std::exp (std::log (a.ratios[k]) * (1.0 - t) + std::log (b.ratios[k]) * t);
            out.levels[k]  = a.levels[k]  * (1.0 - t) + b.levels[k]  * t;
            out.lengths[k] = a.lengths[k] * (1.0 - t) + b.lengths[k] * t;
        }
        return out;
    }

    /** Conjunto de modos para um valor de Character entre 0 e 1. */
    inline ModeSet forCharacter (double character)
    {
        if (character <= 0.5)
            return blend (tongue(), handpan(), character * 2.0);

        return blend (handpan(), crystal(), (character - 0.5) * 2.0);
    }
}
