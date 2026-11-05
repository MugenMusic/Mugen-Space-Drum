#include "SampleBank.h"

namespace
{
    constexpr double twoPi = juce::MathConstants<double>::twoPi;

    /** Ressoador de dois polos: um "modo" de vibração do metal. */
    struct Resonator
    {
        double a1 = 0.0, a2 = 0.0, gain = 0.0, y1 = 0.0, y2 = 0.0;

        Resonator (double frequency, double t60, double level, double sampleRate)
        {
            const double r = std::exp (-6.907755 / (juce::jmax (0.005, t60) * sampleRate));
            const double w = twoPi * frequency / sampleRate;
            a1 = 2.0 * r * std::cos (w);
            a2 = r * r;
            gain = level * std::sin (w);
        }

        inline double process (double x) noexcept
        {
            const double y = a1 * y1 - a2 * y2 + gain * x;
            y2 = y1;
            y1 = y;
            return y;
        }
    };

    struct ModeSpec
    {
        double ratio, t60, level;
    };

    /** Passa a excitação (o "toque") por todos os modos e soma o resultado. */
    void renderModes (std::vector<float>& out, const std::vector<double>& excitation,
                      const std::vector<ModeSpec>& modes, double frequency, double sampleRate)
    {
        for (const auto& m : modes)
        {
            const double f = frequency * m.ratio;
            if (f >= sampleRate * 0.45 || f < 20.0)
                continue;

            Resonator res (f, m.t60, m.level, sampleRate);
            for (size_t i = 0; i < out.size(); ++i)
            {
                const double x = i < excitation.size() ? excitation[i] : 0.0;
                out[i] += (float) res.process (x);
            }
        }
    }

    std::vector<double> halfSinePulse (double seconds, double sampleRate)
    {
        const int length = juce::jmax (2, (int) (seconds * sampleRate));
        std::vector<double> pulse ((size_t) length);
        for (int i = 0; i < length; ++i)
            pulse[(size_t) i] = std::sin (juce::MathConstants<double>::pi * i / (length - 1));
        return pulse;
    }

    std::vector<double> raisedCosinePulse (double seconds, double sampleRate)
    {
        const int length = juce::jmax (2, (int) (seconds * sampleRate));
        std::vector<double> pulse ((size_t) length);
        for (int i = 0; i < length; ++i)
            pulse[(size_t) i] = 0.5 - 0.5 * std::cos (twoPi * i / (length - 1));
        return pulse;
    }

    /** Remove DC, normaliza o volume e suaviza o início e o fim do sample. */
    void finalise (std::vector<float>& data, double sampleRate)
    {
        // Filtro passa-alto ~20 Hz
        const double coef = std::exp (-twoPi * 20.0 / sampleRate);
        double prevIn = 0.0, prevOut = 0.0;
        for (auto& s : data)
        {
            const double y = s - prevIn + coef * prevOut;
            prevIn = s;
            prevOut = y;
            s = (float) y;
        }

        float peak = 0.0f;
        for (auto s : data)
            peak = juce::jmax (peak, std::abs (s));

        if (peak > 0.0f)
            for (auto& s : data)
                s *= 0.8f / peak;

        const int fadeIn  = juce::jmin ((int) data.size(), (int) (0.0005 * sampleRate) + 1);
        const int fadeOut = juce::jmin ((int) data.size(), (int) (0.08 * sampleRate));

        for (int i = 0; i < fadeIn; ++i)
            data[(size_t) i] *= (float) i / (float) fadeIn;

        for (int i = 0; i < fadeOut; ++i)
            data[data.size() - 1 - (size_t) i] *= (float) i / (float) fadeOut;
    }

    double lengthScaleFor (double frequency)
    {
        // Notas graves soam mais tempo.
        const double semitonesFromC4 = 12.0 * std::log2 (frequency / 261.6256);
        return juce::jlimit (0.5, 1.6, std::pow (2.0, -semitonesFromC4 / 24.0));
    }
}

// ------------------------------------------------------------------------------
std::vector<float> SampleBank::renderHand (double frequency, double sampleRate, int seed) const
{
    juce::Random random (seed);
    const double scale = lengthScaleFor (frequency);
    std::vector<float> out ((size_t) (1.6 * scale * sampleRate), 0.0f);

    // Palma: impulso arredondado + ruído de pele dos dedos
    auto excitation = halfSinePulse (0.004, sampleRate);
    const int noiseLength = (int) (0.010 * sampleRate);
    excitation.resize (juce::jmax (excitation.size(), (size_t) noiseLength), 0.0);
    for (int i = 0; i < noiseLength; ++i)
        excitation[(size_t) i] += (random.nextDouble() * 2.0 - 1.0) * 0.15 * std::exp (-i / (0.002 * sampleRate));

    // A mão abafa: modos agudos curtos
    const std::vector<ModeSpec> modes {
        { 1.0,  1.4 * scale, 1.0 },  { 2.0,  0.9 * scale, 0.45 }, { 3.0,  0.6 * scale, 0.25 },
        { 2.42, 0.25, 0.10 }, { 3.63, 0.20, 0.08 }, { 4.51, 0.18, 0.06 },
        { 5.27, 0.15, 0.05 }, { 6.38, 0.12, 0.04 }, { 7.85, 0.10, 0.03 }
    };
    renderModes (out, excitation, modes, frequency, sampleRate);

    // Pancada grave da palma: o tom desce rapidamente até à nota
    double phase = 0.0;
    const int thumpLength = juce::jmin ((int) out.size(), (int) (0.5 * sampleRate));
    for (int i = 0; i < thumpLength; ++i)
    {
        const double t = i / sampleRate;
        const double f = frequency * (1.0 + 0.4 * std::exp (-t / 0.006));
        phase += twoPi * f / sampleRate;
        out[(size_t) i] += (float) (std::sin (phase) * 0.35 * std::exp (-t / 0.07));
    }

    // Pele: ruído brilhante muito curto
    Resonator skin (2600.0, 0.02, 1.0, sampleRate);
    const int skinLength = juce::jmin ((int) out.size(), (int) (0.06 * sampleRate));
    for (int i = 0; i < skinLength; ++i)
    {
        const double n = (random.nextDouble() * 2.0 - 1.0) * std::exp (-i / (0.004 * sampleRate));
        out[(size_t) i] += (float) (skin.process (n) * 0.06);
    }

    finalise (out, sampleRate);
    return out;
}

std::vector<float> SampleBank::renderFelt (double frequency, double sampleRate, int) const
{
    const double scale = lengthScaleFor (frequency);
    std::vector<float> out ((size_t) (5.0 * scale * sampleRate), 0.0f);

    // Feltro: contacto longo e macio, os agudos ficam naturalmente redondos
    const auto excitation = raisedCosinePulse (0.007, sampleRate);

    const std::vector<ModeSpec> modes {
        { 1.0,  4.5 * scale, 1.0 },  { 1.0013, 4.2 * scale, 0.25 },
        { 2.0,  3.0 * scale, 0.55 }, { 1.9988, 2.8 * scale, 0.15 },
        { 3.0,  2.0 * scale, 0.35 },
        { 2.38, 1.2, 0.08 }, { 3.57, 1.0, 0.07 }, { 4.46, 0.8, 0.08 },
        { 5.21, 0.6, 0.05 }, { 6.33, 0.5, 0.04 }, { 7.92, 0.4, 0.03 }, { 9.40, 0.3, 0.02 }
    };
    renderModes (out, excitation, modes, frequency, sampleRate);

    finalise (out, sampleRate);
    return out;
}

std::vector<float> SampleBank::renderShell (double frequency, double sampleRate, int seed) const
{
    juce::Random random (seed);
    const double scale = lengthScaleFor (frequency);
    std::vector<float> out ((size_t) (6.0 * scale * sampleRate), 0.0f);

    auto excitation = halfSinePulse (0.0025, sampleRate);
    for (size_t i = 0; i < excitation.size(); ++i)
        excitation[i] += (random.nextDouble() * 2.0 - 1.0) * 0.05;

    // Modos afinados (com "gémeos" que criam batimentos lentos) ...
    std::vector<ModeSpec> modes {
        { 1.0, 5.5 * scale, 1.0 },  { 1.0012, 5.0 * scale, 0.45 },
        { 2.0, 4.0 * scale, 0.6 },  { 0.9991 * 2.0, 3.6 * scale, 0.30 },
        { 3.0, 3.0 * scale, 0.4 },  { 1.0016 * 3.0, 2.8 * scale, 0.20 }
    };

    // ... e dezenas de modos inarmónicos da casca metálica
    for (int i = 0; i < 30; ++i)
    {
        const double ratio = std::exp (std::log (1.25) + random.nextDouble() * (std::log (16.0) - std::log (1.25)));
        const double level = (0.5 + 0.5 * random.nextDouble()) * 0.12 / std::sqrt (ratio);
        const double t60   = 4.0 * scale / std::sqrt (ratio);
        modes.push_back ({ ratio, t60, level });
    }

    renderModes (out, excitation, modes, frequency, sampleRate);
    finalise (out, sampleRate);
    return out;
}

std::vector<float> SampleBank::renderAir (double frequency, double sampleRate, int seed) const
{
    juce::Random random (seed);
    const double scale = lengthScaleFor (frequency);
    const size_t length = (size_t) (4.5 * scale * sampleRate);
    std::vector<float> out (length, 0.0f);

    // Sopro: ruído suave que cresce devagar e se desvanece
    std::vector<double> breath (length);
    double lowpass = 0.0;
    const double lowpassCoef = 1.0 - std::exp (-twoPi * 3000.0 / sampleRate);
    const double decayTime = 3.5 * scale;
    for (size_t i = 0; i < length; ++i)
    {
        const double t = (double) i / sampleRate;
        const double envelope = (1.0 - std::exp (-t / 0.12)) * std::exp (-6.907755 * t / decayTime);
        lowpass += ((random.nextDouble() * 2.0 - 1.0) - lowpass) * lowpassCoef;
        breath[i] = lowpass * envelope;
    }

    // Ressoadores muito estreitos transformam o ar numa nota afinada e etérea
    const std::vector<ModeSpec> modes {
        { 1.0, 1.6, 1.0 }, { 2.0, 1.3, 0.5 }, { 3.0, 1.0, 0.28 }, { 4.02, 0.8, 0.14 }
    };
    renderModes (out, breath, modes, frequency, sampleRate);

    // Um pouco de ar "real" por cima
    for (size_t i = 0; i < length; ++i)
        out[i] += (float) (breath[i] * 0.004);

    finalise (out, sampleRate);
    return out;
}

// ------------------------------------------------------------------------------
void SampleBank::generate (double sampleRate)
{
    if (sampleRate <= 0.0 || isReadyFor (sampleRate))
        return;

    for (int type = 0; type < numTypes; ++type)
    {
        for (size_t r = 0; r < rootNotes.size(); ++r)
        {
            auto& sample = samples[(size_t) type][r];
            sample.rootNote = rootNotes[r];

            const double frequency = juce::MidiMessage::getMidiNoteInHertz (rootNotes[r]);
            const int seed = 1000 * (type + 1) + rootNotes[r];

            switch (type)
            {
                case hand:  sample.data = renderHand  (frequency, sampleRate, seed); break;
                case felt:  sample.data = renderFelt  (frequency, sampleRate, seed); break;
                case shell: sample.data = renderShell (frequency, sampleRate, seed); break;
                case air:   sample.data = renderAir   (frequency, sampleRate, seed); break;
                default: break;
            }
        }
    }

    generatedSampleRate = sampleRate;
}

const SampleBank::Sample* SampleBank::find (int type, int midiNote) const noexcept
{
    const auto& set = samples[(size_t) juce::jlimit (0, numTypes - 1, type)];

    size_t best = 0;
    for (size_t r = 1; r < rootNotes.size(); ++r)
        if (std::abs (midiNote - rootNotes[r]) < std::abs (midiNote - rootNotes[best]))
            best = r;

    return set[best].data.empty() ? nullptr : &set[best];
}
