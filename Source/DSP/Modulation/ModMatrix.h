#pragma once

#include <JuceHeader.h>
#include <array>
#include "LFO.h"

namespace wavefront::dsp
{

/** Sources de modulation. L'ordre correspond à la StringArray des paramètres. */
enum class ModSource
{
    None = 0, Lfo1, Lfo2, Lfo3, Lfo4,
    PathDistance, PathVelocity, PathX, PathY,
    Count
};

/** Destinations de modulation. L'ordre correspond à la StringArray des paramètres. */
enum class ModDest
{
    None = 0, DopplerAmount, WorldScale, GrainSize, GrainPitch,
    GrainDensity, TremDepth, DelayTime, DistDrive, OutputGain,
    Count
};

/**
    Matrice de modulation : 4 LFOs + sources dérivées de la trajectoire,
    routées vers les destinations via des connexions {source, destination,
    profondeur, smoothing}.

    Calculée par bloc (taux contrôle) : chaque destination reçoit une valeur de
    modulation normalisée ~[-1,1], que le processeur applique ensuite à la valeur
    de base du paramètre concerné. Le lissage par slot évite les sauts.
*/
class ModMatrix
{
public:
    static constexpr int kNumLfos  = 4;
    static constexpr int kNumSlots = 8;

    struct LfoSettings { float rate = 1.0f; float depth = 1.0f; LFO::Shape shape = LFO::Shape::Sine; bool bipolar = true; };
    struct Slot        { ModSource source = ModSource::None; ModDest dest = ModDest::None; float depth = 0.0f; float smooth = 0.2f; };

    /** Sources issues de la trajectoire, fournies chaque bloc par le DopplerEngine. */
    struct PathSources { float distance = 0.0f; float velocity = 0.0f; float x = 0.0f; float y = 0.0f; };

    void prepare (double sr)
    {
        sampleRate = sr;
        for (auto& l : lfos) l.prepare (sr);
        slotState.fill (0.0f);
        modByDest.fill (0.0f);
    }

    void reset()
    {
        for (auto& l : lfos) l.reset();
        slotState.fill (0.0f);
        modByDest.fill (0.0f);
    }

    void setLfo (int i, const LfoSettings& s) noexcept
    {
        if (juce::isPositiveAndBelow (i, kNumLfos))
        {
            lfoSettings[(size_t) i] = s;
            lfos[(size_t) i].setShape (s.shape);
            lfos[(size_t) i].setBipolar (s.bipolar);
        }
    }

    void setSlot (int i, const Slot& s) noexcept
    {
        if (juce::isPositiveAndBelow (i, kNumSlots))
            slots[(size_t) i] = s;
    }

    /** Calcule la modulation du bloc. À appeler une fois par bloc audio. */
    void process (int numSamples, const PathSources& path) noexcept
    {
        std::array<float, kNumLfos> lfoVals {};
        for (int i = 0; i < kNumLfos; ++i)
            lfoVals[(size_t) i] = lfos[(size_t) i].advanceBlock (lfoSettings[(size_t) i].rate, numSamples)
                                  * lfoSettings[(size_t) i].depth;

        modByDest.fill (0.0f);

        const float blockDuration = (float) numSamples / (float) sampleRate;

        for (int i = 0; i < kNumSlots; ++i)
        {
            const auto& slot = slots[(size_t) i];
            const bool  activeSlot = (slot.source != ModSource::None && slot.dest != ModDest::None);

            const float target = activeSlot ? sourceValue (slot.source, lfoVals, path) * slot.depth
                                            : 0.0f;

            // Lissage par slot (one-pole à taux bloc) : temps de montée ~ smooth.
            const float timeConstant = juce::jmap (slot.smooth, 0.0f, 1.0f, 0.001f, 0.5f);
            const float alpha = 1.0f - std::exp (-blockDuration / juce::jmax (1.0e-5f, timeConstant));
            float& state = slotState[(size_t) i];
            state += alpha * (target - state);

            if (activeSlot)
            {
                const int d = (int) slot.dest;
                if (d > 0 && d < (int) ModDest::Count)
                    modByDest[(size_t) d] += state;
            }
        }

        // Bornage doux.
        for (auto& m : modByDest)
            m = juce::jlimit (-1.5f, 1.5f, m);
    }

    /** Valeur de modulation normalisée pour une destination (~[-1.5,1.5]). */
    float getModulation (ModDest dest) const noexcept
    {
        return modByDest[(size_t) dest];
    }

private:
    static float sourceValue (ModSource s, const std::array<float, kNumLfos>& lfoVals,
                              const PathSources& path) noexcept
    {
        switch (s)
        {
            case ModSource::Lfo1: return lfoVals[0];
            case ModSource::Lfo2: return lfoVals[1];
            case ModSource::Lfo3: return lfoVals[2];
            case ModSource::Lfo4: return lfoVals[3];
            case ModSource::PathDistance: return juce::jlimit (-1.0f, 1.0f, path.distance * 2.0f - 1.0f);
            case ModSource::PathVelocity: return juce::jlimit (-1.0f, 1.0f, path.velocity);
            case ModSource::PathX: return juce::jlimit (-1.0f, 1.0f, path.x);
            case ModSource::PathY: return juce::jlimit (-1.0f, 1.0f, path.y);
            case ModSource::None:
            case ModSource::Count:
            default: return 0.0f;
        }
    }

    double sampleRate = 44100.0;
    std::array<LFO, kNumLfos> lfos;
    std::array<LfoSettings, kNumLfos> lfoSettings;
    std::array<Slot, kNumSlots> slots;
    std::array<float, kNumSlots> slotState {};
    std::array<float, (size_t) ModDest::Count> modByDest {};
};

} // namespace wavefront::dsp
