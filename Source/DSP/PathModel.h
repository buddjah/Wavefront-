#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <vector>

namespace wavefront::dsp
{

/**
    Trajectoire 2D de la source sonore, partagée entre l'UI (Path Editor, thread
    message = écrivain) et le thread audio (lecteur).

    Modèle :
      - des points de contrôle en coordonnées normalisées [-1, 1] définissent une
        courbe fermée (spline de Catmull-Rom) ;
      - la courbe est pré-échantillonnée dans une LUT (table) pour que le thread
        audio n'ait qu'à faire une interpolation linéaire — pas de calcul de
        spline en temps réel ;
      - double buffer + index atomique = communication lock-free : l'UI écrit la
        LUT inactive puis publie l'index (release), l'audio lit l'index (acquire).

    Un seul écrivain (thread message) et un seul lecteur (thread audio).
*/
class PathModel
{
public:
    static constexpr int   kLutSize = 2048;
    using Point = juce::Point<float>;

    PathModel()
    {
        setDefaultCircle();
        rebuild();
    }

    // ---- Côté UI (thread message) --------------------------------------

    /** Remplace les points de contrôle puis reconstruit la LUT. */
    void setControlPoints (const std::vector<Point>& pts)
    {
        if (pts.size() >= 3)
            controlPoints = pts;
        rebuild();
    }

    const std::vector<Point>& getControlPoints() const noexcept { return controlPoints; }

    void moveControlPoint (int index, Point newPos)
    {
        if (juce::isPositiveAndBelow (index, (int) controlPoints.size()))
        {
            controlPoints[(size_t) index] = clampToField (newPos);
            rebuild();
        }
    }

    void setDefaultCircle()
    {
        controlPoints.clear();
        const int n = 6;
        const float r = 0.72f;
        for (int i = 0; i < n; ++i)
        {
            const float a = juce::MathConstants<float>::twoPi * (float) i / (float) n;
            controlPoints.push_back ({ r * std::cos (a), r * std::sin (a) });
        }
    }

    /** Recalcule la LUT inactive à partir des points de contrôle et la publie. */
    void rebuild()
    {
        const int next = 1 - active.load (std::memory_order_relaxed);
        auto& lut = luts[(size_t) next];

        const int n = (int) controlPoints.size();
        if (n < 3)
            return;

        for (int j = 0; j < kLutSize; ++j)
        {
            const float t = (float) j / (float) kLutSize * (float) n; // [0, n)
            const int   seg = (int) t;
            const float local = t - (float) seg;

            const Point p0 = controlPoints[(size_t) ((seg - 1 + n) % n)];
            const Point p1 = controlPoints[(size_t) (seg % n)];
            const Point p2 = controlPoints[(size_t) ((seg + 1) % n)];
            const Point p3 = controlPoints[(size_t) ((seg + 2) % n)];

            lut[(size_t) j] = catmullRom (p0, p1, p2, p3, local);
        }

        active.store (next, std::memory_order_release);
    }

    // ---- Côté audio (thread audio) -------------------------------------

    /** Position de la source à la phase donnée (phase01 dans [0,1), enroulée). */
    inline Point evaluate (float phase01) const noexcept
    {
        const auto& lut = luts[(size_t) active.load (std::memory_order_acquire)];

        phase01 -= std::floor (phase01);
        const float fpos = phase01 * (float) kLutSize;
        const int   i0 = (int) fpos;
        const float f  = fpos - (float) i0;
        const int   i1 = (i0 + 1) & (kLutSize - 1);

        return lut[(size_t) (i0 & (kLutSize - 1))] * (1.0f - f) + lut[(size_t) i1] * f;
    }

private:
    static Point clampToField (Point p)
    {
        return { juce::jlimit (-1.0f, 1.0f, p.x), juce::jlimit (-1.0f, 1.0f, p.y) };
    }

    static Point catmullRom (Point p0, Point p1, Point p2, Point p3, float t)
    {
        const float t2 = t * t;
        const float t3 = t2 * t;
        return (p1 * 2.0f
                + (p2 - p0) * t
                + (p0 * 2.0f - p1 * 5.0f + p2 * 4.0f - p3) * t2
                + (p1 * 3.0f - p0 - p2 * 3.0f + p3) * t3) * 0.5f;
    }

    std::vector<Point> controlPoints;
    std::array<std::array<Point, (size_t) kLutSize>, 2> luts {};
    std::atomic<int> active { 0 };
};

} // namespace wavefront::dsp
