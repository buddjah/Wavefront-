#pragma once

#include <JuceHeader.h>
#include "../HQFractionalDelay.h" // lagrange4

namespace wavefront::dsp
{

/**
    Lecteur d'échantillons (« sampler de fichiers »).

    Charge un fichier audio (ou une ressource factory en mémoire) et le rejoue
    en boucle. Sa sortie sert de *source* injectée dans la chaîne : le sample
    « voyage » ensuite sur la trajectoire via le moteur Doppler.

    Communication thread-safe : le chargement (thread message) publie un nouvel
    échantillon sous SpinLock ; le thread audio récupère une référence comptée
    sous try-lock (silence bref si un chargement est en cours). La lecture réutilise
    l'interpolateur de Lagrange (lagrange4) pour la qualité au pitch.
*/
class SamplerEngine
{
public:
    struct Sample : public juce::ReferenceCountedObject
    {
        using Ptr = juce::ReferenceCountedObjectPtr<Sample>;
        juce::AudioBuffer<float> data;
        double sourceSampleRate = 44100.0;
        juce::String name;
    };

    struct Parameters
    {
        bool  enabled = false;
        float gainDb  = -6.0f;
        bool  loop    = true;
        float pitchSt = 0.0f;
    };

    void prepare (double hostSr, int numChannels)
    {
        hostSampleRate = hostSr;
        numCh = juce::jlimit (1, 2, numChannels);
        playPos = 0.0;
    }

    void setParameters (const Parameters& p) noexcept { params = p; }

    /** Publie un nouvel échantillon (thread message). */
    void setSample (Sample::Ptr s)
    {
        const juce::SpinLock::ScopedLockType l (lock);
        current = std::move (s);
        ++generation;
    }

    juce::String getLoadedName() const
    {
        const juce::SpinLock::ScopedLockType l (lock);
        return current != nullptr ? current->name : juce::String();
    }

    /** Ajoute l'audio du sampler dans `dest` (thread audio). */
    void render (juce::AudioBuffer<float>& dest) noexcept
    {
        if (! params.enabled)
            return;

        Sample::Ptr s;
        int gen = 0;
        {
            const juce::SpinLock::ScopedTryLockType l (lock);
            if (! l.isLocked())
                return; // chargement en cours : on saute ce bloc
            s = current;
            gen = generation;
        }
        if (s == nullptr || s->data.getNumSamples() < 4)
            return;

        if (gen != lastGeneration)
        {
            lastGeneration = gen;
            playPos = 0.0;
        }

        const int    len = s->data.getNumSamples();
        const int    srcCh = s->data.getNumChannels();
        const int    dstCh = juce::jmin (numCh, dest.getNumChannels());
        const int    numSamples = dest.getNumSamples();
        const double ratio = (s->sourceSampleRate / hostSampleRate)
                             * std::pow (2.0, (double) params.pitchSt / 12.0);
        const float  gain = juce::Decibels::decibelsToGain (params.gainDb);

        for (int i = 0; i < numSamples; ++i)
        {
            if (playPos >= (double) len)
            {
                if (params.loop) playPos -= (double) len;
                else break;
            }

            const int   i0 = (int) playPos;
            const float f  = (float) (playPos - (double) i0);

            for (int ch = 0; ch < dstCh; ++ch)
            {
                const int sc = juce::jmin (ch, srcCh - 1);
                const float* d = s->data.getReadPointer (sc);
                // Points aux positions relatives -1, 0, +1, +2 ; f dans [0,1]
                // situe la lecture entre x0 (i0) et x1 (i0+1) : sens avant.
                const float xm1 = d[wrap (i0 - 1, len)];
                const float x0  = d[wrap (i0,     len)];
                const float x1  = d[wrap (i0 + 1, len)];
                const float x2  = d[wrap (i0 + 2, len)];
                const float v = lagrange4 (xm1, x0, x1, x2, f) * gain;
                dest.addSample (ch, i, v);
            }
            playPos += ratio;
        }
    }

    void reset() noexcept { playPos = 0.0; }

private:
    static int wrap (int i, int len) noexcept
    {
        i %= len;
        return i < 0 ? i + len : i;
    }

    double hostSampleRate = 44100.0;
    int    numCh = 2;
    double playPos = 0.0;
    int    lastGeneration = -1;

    Parameters params;
    mutable juce::SpinLock lock;
    Sample::Ptr current;
    int generation = 0;
};

} // namespace wavefront::dsp
