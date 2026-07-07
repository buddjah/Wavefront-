#pragma once

#include <JuceHeader.h>
#include <array>
#include "../HQFractionalDelay.h"

namespace wavefront::dsp
{

/**
    Moteur granulaire.

    - capture continue du signal d'entrée dans une ligne à retard fractionnaire
      (réutilisation de HQFractionalDelay -> même interpolateur Lagrange HQ) ;
    - grains fenêtrés (Hann) lus à hauteur variable (pitch) via retard variable ;
    - position de lecture synchronisée au playhead de la trajectoire (scan) ;
    - normalisation par recouvrement (moyenne pondérée par la somme des fenêtres)
      pour une amplitude stable quelle que soit la densité.
*/
class GranularEngine
{
public:
    struct Parameters
    {
        bool  enabled  = false;
        float mix      = 0.5f;
        float grainMs  = 80.0f;
        float density  = 30.0f;  // grains / s
        float spray    = 0.2f;   // 0..1
        float pitchSt  = 0.0f;   // demi-tons
    };

    void prepare (double sr, int numChannels)
    {
        sampleRate = sr;
        numCh = juce::jlimit (1, 2, numChannels);

        const int capSamples = (int) std::ceil (sampleRate * 2.5); // ~2,5 s de capture
        for (auto& c : capture) c.prepare (capSamples);
        maxDelay = (float) capture[0].getMaxDelaySamples();

        reset();
    }

    void reset()
    {
        for (auto& c : capture) c.reset();
        for (auto& g : grains) g.active = false;
        spawnPhase = 0.0;
    }

    void setParameters (const Parameters& p) noexcept { params = p; }

    void process (juce::AudioBuffer<float>& buffer, float pathPhase) noexcept
    {
        const int numSamples = buffer.getNumSamples();
        const int channels = juce::jmin (numCh, buffer.getNumChannels());

        if (! params.enabled || params.mix <= 0.0f)
        {
            // On continue tout de même à capturer pour éviter un « trou » au
            // réengagement, mais sans produire de grains.
            for (int i = 0; i < numSamples; ++i)
                for (int ch = 0; ch < channels; ++ch)
                    capture[(size_t) ch].pushSample (buffer.getSample (ch, i));
            return;
        }

        const float grainLen  = juce::jmax (4.0f, params.grainMs * 0.001f * (float) sampleRate);
        const float pitchRatio = std::pow (2.0f, params.pitchSt / 12.0f);
        const float scanRange = 0.5f * (float) sampleRate; // le scan couvre ~0,5 s
        const float sprayRange = params.spray * 0.5f * (float) sampleRate;
        const double grainsPerSample = (double) params.density / sampleRate;
        const float mix = params.mix;

        for (int i = 0; i < numSamples; ++i)
        {
            float in[2] = { 0.0f, 0.0f };
            for (int ch = 0; ch < channels; ++ch)
            {
                in[ch] = buffer.getSample (ch, i);
                capture[(size_t) ch].pushSample (in[ch]);
            }

            // Ordonnancement des grains (densité constante).
            spawnPhase += grainsPerSample;
            while (spawnPhase >= 1.0)
            {
                spawnPhase -= 1.0;
                spawnGrain (grainLen, pitchRatio, scanRange, sprayRange, pathPhase);
            }

            // Accumulation des grains actifs.
            float acc[2] = { 0.0f, 0.0f };
            float windowSum = 0.0f;

            for (auto& g : grains)
            {
                if (! g.active) continue;

                const float w = hann ((float) g.age / g.length);
                for (int ch = 0; ch < channels; ++ch)
                    acc[ch] += w * capture[(size_t) ch].readAt ((float) g.delay);
                windowSum += w;

                g.delay += g.delayInc;
                if (g.delay < 1.0)        g.delay = 1.0;
                if (g.delay > maxDelay)   g.delay = maxDelay;

                if (++g.age >= (int) g.length)
                    g.active = false;
            }

            const float norm = windowSum > 1.0e-4f ? (1.0f / windowSum) : 0.0f;
            for (int ch = 0; ch < channels; ++ch)
            {
                const float grainOut = acc[ch] * norm;
                buffer.setSample (ch, i, in[ch] * (1.0f - mix) + grainOut * mix);
            }
        }
    }

private:
    struct Grain
    {
        bool   active = false;
        double delay = 1.0;
        double delayInc = 0.0;
        int    age = 0;
        float  length = 1.0f;
    };

    static float hann (float x) noexcept // x dans [0,1]
    {
        return 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * juce::jlimit (0.0f, 1.0f, x));
    }

    void spawnGrain (float grainLen, float pitchRatio, float scanRange,
                     float sprayRange, float pathPhase) noexcept
    {
        // Recherche d'un grain libre ; sinon on vole le plus ancien.
        Grain* slot = nullptr;
        int oldestAge = -1;
        for (auto& g : grains)
        {
            if (! g.active) { slot = &g; break; }
            if (g.age > oldestAge) { oldestAge = g.age; slot = &g; }
        }
        if (slot == nullptr) return;

        // Marge pour que le retard reste dans [1, maxDelay] sur toute la vie
        // du grain (si pitch > 1, le retard décroît).
        const float shrink = juce::jmax (0.0f, pitchRatio - 1.0f) * grainLen;
        const float scan   = pathPhase * scanRange; // sync playhead
        const float spray  = random.nextFloat() * sprayRange;
        float startDelay   = 64.0f + shrink + scan + spray;
        startDelay = juce::jlimit (1.0f, maxDelay, startDelay);

        slot->active   = true;
        slot->age      = 0;
        slot->length   = grainLen;
        slot->delay    = startDelay;
        slot->delayInc = 1.0 - (double) pitchRatio;
    }

    double sampleRate = 44100.0;
    int    numCh = 2;
    float  maxDelay = 0.0f;
    double spawnPhase = 0.0;

    std::array<HQFractionalDelay, 2> capture;
    static constexpr int kMaxGrains = 128;
    std::array<Grain, kMaxGrains> grains;
    juce::Random random;
    Parameters params;
};

} // namespace wavefront::dsp
