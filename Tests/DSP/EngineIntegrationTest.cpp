// Test d'intégration DSP : rend de l'audio à travers DopplerEngine et
// GranularEngine et vérifie que la sortie est finie, bornée, non silencieuse,
// et que le Doppler produit bien une variation de hauteur.
//
// Nécessite JUCE (AudioBuffer, dsp::Oversampling, Point) -> cible console app.

#include <JuceHeader.h>
#include "DSP/PathModel.h"
#include "DSP/DopplerEngine.h"
#include "DSP/Granular/GranularEngine.h"
#include "DSP/Effects/EffectsChain.h"
#include "DSP/Sampler/SamplerEngine.h"

#include <cmath>
#include <cstdio>
#include <vector>

namespace
{
int failures = 0;
void check (bool cond, const char* what)
{
    std::printf ("[%s] %s\n", cond ? " OK " : "FAIL", what);
    if (! cond) ++failures;
}

// Compte les passages par zéro (montants) -> proxy de la fréquence.
int upwardZeroCrossings (const std::vector<float>& x)
{
    int n = 0;
    for (size_t i = 1; i < x.size(); ++i)
        if (x[i - 1] <= 0.0f && x[i] > 0.0f) ++n;
    return n;
}

struct RenderStats { bool allFinite = true; float peak = 0.0f; double rms = 0.0; };

RenderStats analyze (const std::vector<float>& x)
{
    RenderStats s;
    double sq = 0.0;
    for (float v : x)
    {
        if (! std::isfinite (v)) s.allFinite = false;
        s.peak = std::max (s.peak, std::fabs (v));
        sq += (double) v * v;
    }
    s.rms = std::sqrt (sq / std::max<size_t> (1, x.size()));
    return s;
}
} // namespace

int main()
{
    using namespace wavefront::dsp;

    const double sr = 48000.0;
    const int    block = 256;
    const int    numBlocks = 400; // ~2,1 s
    const float  inFreq = 300.0f;

    // ---------------------------------------------------------- Doppler
    {
        PathModel path;
        DopplerEngine eng;
        eng.prepare (sr, block, 2, &path);

        DopplerEngine::Parameters p;
        p.pathRateHz = 3.0f;   // orbite rapide -> Doppler marqué
        p.worldScaleM = 30.0f;
        p.amount = 1.0f;
        p.distanceAtten = 0.0f; // isole l'effet de hauteur (pas d'atténuation)
        p.airAbsorption = 0.0f;
        eng.setParameters (p);

        juce::AudioBuffer<float> buf (2, block);
        std::vector<float> out;
        out.reserve ((size_t) (numBlocks * block));

        double ph = 0.0;
        const double inc = juce::MathConstants<double>::twoPi * inFreq / sr;

        for (int b = 0; b < numBlocks; ++b)
        {
            for (int i = 0; i < block; ++i)
            {
                const float s = (float) std::sin (ph);
                ph += inc;
                buf.setSample (0, i, s);
                buf.setSample (1, i, s);
            }
            eng.process (buf);
            for (int i = 0; i < block; ++i)
                out.push_back (buf.getSample (0, i));
        }

        auto st = analyze (out);
        check (st.allFinite, "Doppler: sortie entierement finie");
        check (st.peak < 8.0f, "Doppler: sortie bornee");
        check (st.rms > 1.0e-3, "Doppler: sortie non silencieuse");

        // Mesure la fréquence instantanée (passages par zéro) sur des fenêtres
        // glissantes courtes : l'écart max/min traduit la modulation de hauteur.
        const int win = 2048;
        int zcMin = 1 << 30, zcMax = 0;
        for (int start = 0; start + win <= (int) out.size(); start += win)
        {
            std::vector<float> w (out.begin() + start, out.begin() + start + win);
            const int zc = upwardZeroCrossings (w);
            zcMin = std::min (zcMin, zc);
            zcMax = std::max (zcMax, zc);
        }
        // Fréquence d'entrée sur 2048 échantillons @48k ≈ 12,8 cycles.
        // Un Doppler marqué doit faire varier ce compte de plusieurs unités.
        check (zcMax - zcMin >= 3, "Doppler: modulation de hauteur significative");
        std::printf ("      (zc min=%d, max=%d, ecart=%d)\n", zcMin, zcMax, zcMax - zcMin);
    }

    // --------------------------------------------------------- Granulaire
    {
        GranularEngine gr;
        gr.prepare (sr, 2);
        GranularEngine::Parameters gp;
        gp.enabled = true;
        gp.mix = 1.0f;
        gp.grainMs = 60.0f;
        gp.density = 40.0f;
        gp.spray = 0.3f;
        gp.pitchSt = 7.0f;
        gr.setParameters (gp);

        juce::AudioBuffer<float> buf (2, block);
        std::vector<float> out;
        double ph = 0.0;
        const double inc = juce::MathConstants<double>::twoPi * inFreq / sr;

        for (int b = 0; b < numBlocks; ++b)
        {
            for (int i = 0; i < block; ++i)
            {
                const float s = (float) std::sin (ph);
                ph += inc;
                buf.setSample (0, i, s);
                buf.setSample (1, i, s);
            }
            gr.process (buf, (float) (b % 100) / 100.0f);
            for (int i = 0; i < block; ++i)
                out.push_back (buf.getSample (0, i));
        }

        auto st = analyze (out);
        check (st.allFinite, "Granulaire: sortie entierement finie");
        check (st.peak < 8.0f, "Granulaire: sortie bornee");
        check (st.rms > 1.0e-3, "Granulaire: sortie non silencieuse");
    }

    // -------------------------------------------------- Chaine d'effets
    {
        EffectsChain fx;
        fx.prepare (sr, block, 2);
        EffectsChain::Parameters ep;
        ep.tremOn = true; ep.tremRate = 5.0f; ep.tremDepth = 0.6f;
        ep.dlyOn = true;  ep.dlyTimeMs = 120.0f; ep.dlyFeedback = 0.4f; ep.dlyMix = 0.4f;
        ep.distOn = true; ep.distDrive = 0.6f; ep.distMix = 0.8f;
        ep.eqOn = true;   ep.eqLowGain = 3.0f; ep.eqMidGain = -4.0f; ep.eqMidFreq = 1500.0f; ep.eqHighGain = 2.0f;
        ep.compOn = true; ep.compThreshold = -20.0f; ep.compRatio = 4.0f;
        fx.setParameters (ep);

        juce::AudioBuffer<float> buf (2, block);
        std::vector<float> out;
        double ph = 0.0;
        const double inc = juce::MathConstants<double>::twoPi * inFreq / sr;

        for (int b = 0; b < numBlocks; ++b)
        {
            for (int i = 0; i < block; ++i)
            {
                const float s = 0.5f * (float) std::sin (ph);
                ph += inc;
                buf.setSample (0, i, s);
                buf.setSample (1, i, s);
            }
            fx.process (buf);
            for (int i = 0; i < block; ++i)
                out.push_back (buf.getSample (0, i));
        }

        auto st = analyze (out);
        check (st.allFinite, "Effets: sortie entierement finie (tous actifs)");
        check (st.peak < 8.0f, "Effets: sortie bornee");
        check (st.rms > 1.0e-3, "Effets: sortie non silencieuse");
    }

    // ----------------------------------------------------------- Sampler
    {
        // Échantillon synthétique (sinus 1 s @ sr) chargé directement.
        auto sample = new SamplerEngine::Sample();
        const int slen = (int) sr;
        sample->data.setSize (1, slen);
        for (int i = 0; i < slen; ++i)
            sample->data.setSample (0, i, (float) std::sin (
                juce::MathConstants<double>::twoPi * 220.0 * i / sr));
        sample->sourceSampleRate = sr;
        sample->name = "test";

        SamplerEngine smp;
        smp.prepare (sr, 2);
        smp.setSample (SamplerEngine::Sample::Ptr (sample));
        SamplerEngine::Parameters spar;
        spar.enabled = true; spar.gainDb = 0.0f; spar.loop = true; spar.pitchSt = 5.0f;
        smp.setParameters (spar);

        juce::AudioBuffer<float> buf (2, block);
        std::vector<float> out;
        for (int b = 0; b < 200; ++b) // > 1 s -> exerce le bouclage
        {
            buf.clear();
            smp.render (buf); // ajoute dans un buffer vide
            for (int i = 0; i < block; ++i)
                out.push_back (buf.getSample (0, i));
        }

        auto st = analyze (out);
        check (st.allFinite, "Sampler: sortie entierement finie");
        check (st.peak < 8.0f, "Sampler: sortie bornee");
        check (st.rms > 1.0e-3, "Sampler: lecture bouclee non silencieuse");
    }

    if (failures == 0)
        std::printf ("Integration DSP : tous les tests passent.\n");
    return failures == 0 ? 0 : 1;
}
