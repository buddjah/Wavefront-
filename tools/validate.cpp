// Validation « torture test » headless du plugin (façon pluginval, mais sans
// dépendance externe). Instancie WavefrontAudioProcessor et le soumet à :
//   - plusieurs fréquences d'échantillonnage et tailles de bloc ;
//   - entrées pathologiques (silence, impulsion, bruit blanc, sinus, DC) ;
//   - balayages de paramètres aléatoires en cours de traitement ;
//   - round-trip de sauvegarde/restauration d'état ;
// et vérifie qu'aucune sortie n'est NaN/Inf ni déraisonnablement grande.

#include <JuceHeader.h>
#include "PluginProcessor.h"

#include <cstdio>
#include <vector>

namespace
{
int failures = 0;
void check (bool cond, const juce::String& what)
{
    std::printf ("[%s] %s\n", cond ? " OK " : "FAIL", what.toRawUTF8());
    if (! cond) ++failures;
}

bool bufferIsSane (const juce::AudioBuffer<float>& b)
{
    for (int ch = 0; ch < b.getNumChannels(); ++ch)
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            const float v = b.getSample (ch, i);
            if (! std::isfinite (v) || std::fabs (v) > 32.0f)
                return false;
        }
    return true;
}

void fillInput (juce::AudioBuffer<float>& b, int kind, double& phase, juce::Random& rng)
{
    const double inc = juce::MathConstants<double>::twoPi * 220.0 / 48000.0;
    for (int i = 0; i < b.getNumSamples(); ++i)
    {
        float s = 0.0f;
        switch (kind)
        {
            case 0: s = 0.0f; break;                                   // silence
            case 1: s = (i == 0 ? 1.0f : 0.0f); break;                 // impulsion
            case 2: s = rng.nextFloat() * 2.0f - 1.0f; break;          // bruit blanc
            case 3: s = (float) std::sin (phase); phase += inc; break; // sinus
            case 4: s = 0.9f; break;                                   // DC
            default: break;
        }
        for (int ch = 0; ch < b.getNumChannels(); ++ch)
            b.setSample (ch, i, s);
    }
}
} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI guiInit;
    juce::Random rng (12345);

    const int sampleRates[] = { 44100, 48000, 96000 };
    const int blockSizes[]  = { 32, 128, 512, 1024 };

    for (int sr : sampleRates)
    {
        for (int bs : blockSizes)
        {
            wavefront::WavefrontAudioProcessor proc;
            proc.setPlayConfigDetails (2, 2, (double) sr, bs);
            proc.prepareToPlay ((double) sr, bs);

            auto* params = &proc.getParameters();
            juce::MidiBuffer midi;
            double phase = 0.0;
            bool sane = true;

            for (int kind = 0; kind <= 4 && sane; ++kind)
            {
                for (int blk = 0; blk < 40 && sane; ++blk)
                {
                    // Balaie des paramètres au hasard pour stresser les moteurs.
                    if (blk % 5 == 0)
                        for (auto* p : *params)
                            if (rng.nextFloat() < 0.15f)
                                if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
                                    rp->setValueNotifyingHost (rng.nextFloat());

                    juce::AudioBuffer<float> buf (2, bs);
                    fillInput (buf, kind, phase, rng);
                    proc.processBlock (buf, midi);
                    sane = bufferIsSane (buf);
                }
            }

            check (sane, "Traitement sain @ " + juce::String (sr) + " Hz / bloc " + juce::String (bs));
            proc.releaseResources();
        }
    }

    // Round-trip d'état.
    {
        wavefront::WavefrontAudioProcessor a;
        a.prepareToPlay (48000.0, 256);
        for (auto* p : a.getParameters())
            if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
                rp->setValueNotifyingHost (rng.nextFloat());

        juce::MemoryBlock state;
        a.getStateInformation (state);

        wavefront::WavefrontAudioProcessor b;
        b.prepareToPlay (48000.0, 256);
        b.setStateInformation (state.getData(), (int) state.getSize());

        juce::MemoryBlock state2;
        b.getStateInformation (state2);
        check (state == state2, "Round-trip sauvegarde/restauration d'etat");
    }

    if (failures == 0)
        std::printf ("Validation plugin : tout est OK.\n");
    else
        std::printf ("Validation plugin : %d echec(s).\n", failures);

    return failures == 0 ? 0 : 1;
}
