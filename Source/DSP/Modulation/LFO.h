#pragma once

#include <JuceHeader.h>
#include <cmath>

namespace wavefront::dsp
{

/**
    Oscillateur basse fréquence pour la matrice de modulation.

    Formes : sinus, triangle, dent de scie, carré, sample & hold.
    Sortie bipolaire [-1,1] ou unipolaire [0,1]. Avance par blocs (taux contrôle).
*/
class LFO
{
public:
    enum class Shape { Sine = 0, Triangle, Saw, Square, SampleHold };

    void prepare (double sr)
    {
        sampleRate = sr;
        phase = 0.0;
        shValue = random.nextFloat() * 2.0f - 1.0f;
    }

    void reset()
    {
        phase = 0.0;
        shValue = random.nextFloat() * 2.0f - 1.0f;
    }

    void setShape (Shape s)   noexcept { shape = s; }
    void setBipolar (bool b)  noexcept { bipolar = b; }

    /** Avance la phase de `numSamples` échantillons et renvoie la valeur courante. */
    float advanceBlock (float rateHz, int numSamples) noexcept
    {
        const double prevPhase = phase;
        phase += (double) rateHz * (double) numSamples / sampleRate;

        // Détection de tour complet (pour le sample & hold).
        if (std::floor (phase) > std::floor (prevPhase) || phase >= 1.0)
            shValue = random.nextFloat() * 2.0f - 1.0f;

        if (phase >= 1.0) phase -= std::floor (phase);

        float v = raw ((float) phase); // [-1,1]
        return bipolar ? v : (v * 0.5f + 0.5f);
    }

    float getPhase() const noexcept { return (float) phase; }

private:
    float raw (float p) const noexcept
    {
        switch (shape)
        {
            case Shape::Sine:     return std::sin (juce::MathConstants<float>::twoPi * p);
            case Shape::Triangle: return 4.0f * std::fabs (p - 0.5f) - 1.0f;
            case Shape::Saw:      return 2.0f * p - 1.0f;
            case Shape::Square:   return p < 0.5f ? 1.0f : -1.0f;
            case Shape::SampleHold: return shValue;
            default:              return 0.0f;
        }
    }

    double sampleRate = 44100.0;
    double phase = 0.0;
    Shape  shape = Shape::Sine;
    bool   bipolar = true;
    float  shValue = 0.0f;
    juce::Random random;
};

} // namespace wavefront::dsp
