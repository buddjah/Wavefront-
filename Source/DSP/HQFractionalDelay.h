#pragma once

#include <vector>
#include <cmath>
#include <cstddef>

namespace wavefront::dsp
{

/** Noyau d'interpolation de Lagrange 4 points (3e ordre).
    f dans [0,1] situe le point recherché entre x0 et x1 ; xm1 et x2 sont les
    voisins. Partagé par la ligne à retard et le moteur granulaire. */
inline float lagrange4 (float xm1, float x0, float x1, float x2, float f) noexcept
{
    const float c0 = x0;
    const float c1 = x1 - (1.0f / 3.0f) * xm1 - 0.5f * x0 - (1.0f / 6.0f) * x2;
    const float c2 = 0.5f * (xm1 + x1) - x0;
    const float c3 = (1.0f / 6.0f) * (x2 - xm1) + 0.5f * (x0 - x1);
    return ((c3 * f + c2) * f + c1) * f + c0;
}

/**
    Ligne à retard fractionnaire haute qualité, mono, avec interpolation de
    Lagrange 4 points (3e ordre).

    Utilisée à la fois par le moteur Doppler (retard variable = pitch shift) et,
    plus tard, par le moteur granulaire (lecture à taux variable). Indépendante
    de JUCE et de la plateforme pour rester testable et portable.

    Usage :
        HQFractionalDelay line;
        line.prepare (maxDelaySamples);
        line.pushSample (x);
        float y = line.readAt (delayInSamples); // delay >= 1.0

    Le retard minimum interpolable est 1 échantillon (la formule 4 points a
    besoin d'un échantillon « futur » par rapport au point de lecture).
*/
class HQFractionalDelay
{
public:
    HQFractionalDelay() = default;

    /** Alloue le buffer. maxDelaySamples est arrondi à la puissance de 2
        supérieure pour permettre un masquage rapide de l'index circulaire. */
    void prepare (int maxDelaySamples)
    {
        int required = maxDelaySamples + 4; // marge pour les 4 points
        size_t size = 1;
        while (size < static_cast<size_t> (required))
            size <<= 1;

        buffer.assign (size, 0.0f);
        mask = size - 1;
        writePos = 0;
    }

    void reset()
    {
        std::fill (buffer.begin(), buffer.end(), 0.0f);
        writePos = 0;
    }

    int getMaxDelaySamples() const noexcept
    {
        return static_cast<int> (buffer.size()) - 4;
    }

    /** Écrit un échantillon et avance la tête d'écriture. */
    inline void pushSample (float x) noexcept
    {
        buffer[writePos] = x;
        writePos = (writePos + 1) & mask;
    }

    /** Lit un échantillon à un retard fractionnaire (en échantillons).
        delaySamples est borné à [1, maxDelay]. */
    inline float readAt (float delaySamples) const noexcept
    {
        const float maxD = static_cast<float> (getMaxDelaySamples());
        if (delaySamples < 1.0f)   delaySamples = 1.0f;
        if (delaySamples > maxD)   delaySamples = maxD;

        const int   iD = static_cast<int> (delaySamples);
        const float f  = delaySamples - static_cast<float> (iD);

        // Index du point de lecture « central » (x0), puis ses voisins.
        // readPos pointe sur le dernier échantillon écrit lorsque delay == 1.
        const size_t base = (writePos + buffer.size() - static_cast<size_t> (iD)) & mask;

        const float xm1 = buffer[(base + 1) & mask]; // échantillon le plus récent
        const float x0  = buffer[base];
        const float x1  = buffer[(base + buffer.size() - 1) & mask];
        const float x2  = buffer[(base + buffer.size() - 2) & mask];

        return lagrange4 (xm1, x0, x1, x2, f);
    }

    /** Accès à la taille interne (puissance de 2) — utile aux lecteurs externes
        (moteur granulaire) qui réutilisent le buffer via readAt(). */
    int getBufferSize() const noexcept { return static_cast<int> (buffer.size()); }

private:
    std::vector<float> buffer;
    size_t mask = 0;
    size_t writePos = 0;
};

} // namespace wavefront::dsp
