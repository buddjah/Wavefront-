// Tests unitaires de HQFractionalDelay (interpolation de Lagrange 4 points).
// Indépendant de JUCE : n'inclut que le header DSP.

#include "DSP/HQFractionalDelay.h"

#include <cmath>
#include <cstdio>

using wavefront::dsp::HQFractionalDelay;

namespace
{
int failures = 0;

void check (bool cond, const char* what)
{
    std::printf ("[%s] %s\n", cond ? " OK " : "FAIL", what);
    if (! cond) ++failures;
}

bool approx (float a, float b, float eps = 1.0e-4f)
{
    return std::fabs (a - b) <= eps;
}
} // namespace

int main()
{
    // 1) Retard entier : readAt(D) doit rendre l'échantillon écrit D pushes plus tôt.
    {
        HQFractionalDelay line;
        line.prepare (64);
        for (int i = 0; i <= 10; ++i)
            line.pushSample ((float) i); // dernier écrit = 10

        check (approx (line.readAt (1.0f), 10.0f), "retard entier D=1 -> dernier echantillon");
        check (approx (line.readAt (2.0f),  9.0f), "retard entier D=2");
        check (approx (line.readAt (5.0f),  6.0f), "retard entier D=5");
    }

    // 2) Signal constant : l'interpolation fractionnaire reproduit la constante.
    {
        HQFractionalDelay line;
        line.prepare (64);
        for (int i = 0; i < 40; ++i)
            line.pushSample (0.75f);

        check (approx (line.readAt (3.5f),  0.75f), "constante @ D=3.5");
        check (approx (line.readAt (7.25f), 0.75f), "constante @ D=7.25");
    }

    // 3) Rampe linéaire : Lagrange 3e ordre est exact sur un signal affine.
    {
        HQFractionalDelay line;
        line.prepare (128);
        for (int i = 0; i < 60; ++i)
            line.pushSample ((float) i); // x[n] = n, dernier = 59

        // readAt(D) = valeur écrite D pushes plus tôt = 59 - (D-1) = 60 - D.
        check (approx (line.readAt (2.5f), 60.0f - 2.5f), "rampe lineaire @ D=2.5 (exact)");
        check (approx (line.readAt (9.5f), 60.0f - 9.5f), "rampe lineaire @ D=9.5 (exact)");
    }

    // 4) Bornage : delay < 1 est ramené à 1, delay > max est clampé (pas de crash).
    {
        HQFractionalDelay line;
        line.prepare (16);
        for (int i = 0; i < 8; ++i) line.pushSample (1.0f);
        volatile float a = line.readAt (0.0f);
        volatile float b = line.readAt (1.0e6f);
        check (std::isfinite (a) && std::isfinite (b), "bornage delay -> valeurs finies");
    }

    if (failures == 0)
        std::printf ("HQFractionalDelay : tous les tests passent.\n");
    return failures == 0 ? 0 : 1;
}
