#include "Parameters.h"
#include "ParameterIDs.h"

namespace wavefront::params
{

using APF = juce::AudioParameterFloat;
using APC = juce::AudioParameterChoice;
using APB = juce::AudioParameterBool;
using Range = juce::NormalisableRange<float>;

namespace
{
    // Range avec courbe (skew) déduite d'un point milieu — pratique pour les
    // fréquences / temps où l'on veut plus de résolution en bas.
    Range skewedRange (float lo, float hi, float centre, float step = 0.0f)
    {
        Range r (lo, hi);
        r.setSkewForCentre (centre);
        if (step > 0.0f) r.interval = step;
        return r;
    }

    juce::String dbFormat (float v, int)  { return juce::String (v, 1) + " dB"; }
    juce::String hzFormat (float v, int)  { return v < 1.0f ? juce::String (v, 3) + " Hz"
                                                             : juce::String (v, 2) + " Hz"; }
    juce::String msFormat (float v, int)  { return juce::String (v, 1) + " ms"; }
    juce::String pctFormat(float v, int)  { return juce::String (juce::roundToInt (v * 100.0f)) + " %"; }
    juce::String mFormat  (float v, int)  { return juce::String (v, 1) + " m"; }
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    auto pctAttr = juce::AudioParameterFloatAttributes().withStringFromValueFunction (pctFormat);

    // ---------------------------------------------------------------- Doppler
    {
        using namespace doppler;
        layout.add (std::make_unique<APF> (juce::ParameterID { pathRate, kStateVersion },
            "Path Rate", skewedRange (0.01f, 10.0f, 1.0f), 0.5f,
            juce::AudioParameterFloatAttributes().withStringFromValueFunction (hzFormat)));

        layout.add (std::make_unique<APF> (juce::ParameterID { worldScale, kStateVersion },
            "World Scale", skewedRange (1.0f, 100.0f, 20.0f), 20.0f,
            juce::AudioParameterFloatAttributes().withStringFromValueFunction (mFormat)));

        layout.add (std::make_unique<APF> (juce::ParameterID { speedOfSound, kStateVersion },
            "Speed of Sound", Range (100.0f, 700.0f, 1.0f), 343.0f,
            juce::AudioParameterFloatAttributes().withStringFromValueFunction (
                [] (float v, int) { return juce::String (juce::roundToInt (v)) + " m/s"; })));

        layout.add (std::make_unique<APF> (juce::ParameterID { amount, kStateVersion },
            "Doppler Amount", Range (0.0f, 1.0f), 1.0f, pctAttr));

        layout.add (std::make_unique<APF> (juce::ParameterID { distanceAtten, kStateVersion },
            "Distance Atten.", Range (0.0f, 1.0f), 0.5f, pctAttr));

        layout.add (std::make_unique<APF> (juce::ParameterID { airAbsorption, kStateVersion },
            "Air Absorption", Range (0.0f, 1.0f), 0.4f, pctAttr));

        layout.add (std::make_unique<APF> (juce::ParameterID { listenerX, kStateVersion },
            "Listener X", Range (-1.0f, 1.0f), 0.0f));
        layout.add (std::make_unique<APF> (juce::ParameterID { listenerY, kStateVersion },
            "Listener Y", Range (-1.0f, 1.0f), 0.0f));
    }

    // --------------------------------------------------------------- Granular
    {
        using namespace granular;
        layout.add (std::make_unique<APB> (juce::ParameterID { enable, kStateVersion },
            "Granular Enable", false));
        layout.add (std::make_unique<APF> (juce::ParameterID { mix, kStateVersion },
            "Granular Mix", Range (0.0f, 1.0f), 0.5f, pctAttr));
        layout.add (std::make_unique<APF> (juce::ParameterID { grainMs, kStateVersion },
            "Grain Size", skewedRange (5.0f, 500.0f, 80.0f), 80.0f,
            juce::AudioParameterFloatAttributes().withStringFromValueFunction (msFormat)));
        layout.add (std::make_unique<APF> (juce::ParameterID { density, kStateVersion },
            "Grain Density", skewedRange (1.0f, 200.0f, 30.0f), 30.0f,
            juce::AudioParameterFloatAttributes().withStringFromValueFunction (hzFormat)));
        layout.add (std::make_unique<APF> (juce::ParameterID { spray, kStateVersion },
            "Spray", Range (0.0f, 1.0f), 0.2f, pctAttr));
        layout.add (std::make_unique<APF> (juce::ParameterID { pitch, kStateVersion },
            "Grain Pitch", Range (-24.0f, 24.0f, 0.01f), 0.0f,
            juce::AudioParameterFloatAttributes().withStringFromValueFunction (
                [] (float v, int) { return juce::String (v, 2) + " st"; })));
    }

    // ------------------------------------------------------------------- LFOs
    {
        using namespace mod;
        const juce::StringArray shapes { "Sine", "Triangle", "Saw", "Square", "S&H" };
        for (int i = 0; i < kNumLfos; ++i)
        {
            layout.add (std::make_unique<APF> (juce::ParameterID { lfoRate (i), kStateVersion },
                "LFO " + juce::String (i + 1) + " Rate",
                skewedRange (0.01f, 40.0f, 2.0f), 1.0f,
                juce::AudioParameterFloatAttributes().withStringFromValueFunction (hzFormat)));
            layout.add (std::make_unique<APF> (juce::ParameterID { lfoDepth (i), kStateVersion },
                "LFO " + juce::String (i + 1) + " Depth", Range (0.0f, 1.0f), 1.0f, pctAttr));
            layout.add (std::make_unique<APC> (juce::ParameterID { lfoShape (i), kStateVersion },
                "LFO " + juce::String (i + 1) + " Shape", shapes, 0));
            layout.add (std::make_unique<APB> (juce::ParameterID { lfoBipolar (i), kStateVersion },
                "LFO " + juce::String (i + 1) + " Bipolar", true));
        }
    }

    // -------------------------------------------------------- Matrice de mod.
    {
        using namespace mod;
        const juce::StringArray sources {
            "None", "LFO 1", "LFO 2", "LFO 3", "LFO 4",
            "Path Distance", "Path Velocity", "Path X", "Path Y" };
        const juce::StringArray dests {
            "None", "Doppler Amount", "World Scale", "Grain Size",
            "Grain Pitch", "Grain Density", "Trem Depth", "Delay Time",
            "Dist Drive", "Output Gain" };

        for (int i = 0; i < kNumSlots; ++i)
        {
            layout.add (std::make_unique<APC> (juce::ParameterID { slotSource (i), kStateVersion },
                "Mod " + juce::String (i + 1) + " Src", sources, 0));
            layout.add (std::make_unique<APC> (juce::ParameterID { slotDest (i), kStateVersion },
                "Mod " + juce::String (i + 1) + " Dst", dests, 0));
            layout.add (std::make_unique<APF> (juce::ParameterID { slotDepth (i), kStateVersion },
                "Mod " + juce::String (i + 1) + " Depth", Range (-1.0f, 1.0f), 0.0f, pctAttr));
            layout.add (std::make_unique<APF> (juce::ParameterID { slotSmooth (i), kStateVersion },
                "Mod " + juce::String (i + 1) + " Smooth", Range (0.0f, 1.0f), 0.2f, pctAttr));
        }
    }

    // --------------------------------------------------------------- Effets
    {
        using namespace fx;
        layout.add (std::make_unique<APB> (juce::ParameterID { tremEnable, kStateVersion }, "Tremolo On", false));
        layout.add (std::make_unique<APF> (juce::ParameterID { tremRate, kStateVersion },
            "Tremolo Rate", skewedRange (0.1f, 20.0f, 4.0f), 4.0f,
            juce::AudioParameterFloatAttributes().withStringFromValueFunction (hzFormat)));
        layout.add (std::make_unique<APF> (juce::ParameterID { tremDepth, kStateVersion },
            "Tremolo Depth", Range (0.0f, 1.0f), 0.5f, pctAttr));

        layout.add (std::make_unique<APB> (juce::ParameterID { dlyEnable, kStateVersion }, "Delay On", false));
        layout.add (std::make_unique<APF> (juce::ParameterID { dlyTime, kStateVersion },
            "Delay Time", skewedRange (1.0f, 2000.0f, 300.0f), 300.0f,
            juce::AudioParameterFloatAttributes().withStringFromValueFunction (msFormat)));
        layout.add (std::make_unique<APF> (juce::ParameterID { dlyFeedback, kStateVersion },
            "Delay Feedback", Range (0.0f, 0.98f), 0.35f, pctAttr));
        layout.add (std::make_unique<APF> (juce::ParameterID { dlyMix, kStateVersion },
            "Delay Mix", Range (0.0f, 1.0f), 0.3f, pctAttr));

        layout.add (std::make_unique<APB> (juce::ParameterID { distEnable, kStateVersion }, "Distortion On", false));
        layout.add (std::make_unique<APF> (juce::ParameterID { distDrive, kStateVersion },
            "Drive", Range (0.0f, 1.0f), 0.3f, pctAttr));
        layout.add (std::make_unique<APF> (juce::ParameterID { distMix, kStateVersion },
            "Distortion Mix", Range (0.0f, 1.0f), 1.0f, pctAttr));

        layout.add (std::make_unique<APB> (juce::ParameterID { eqEnable, kStateVersion }, "EQ On", false));
        layout.add (std::make_unique<APF> (juce::ParameterID { eqLowGain, kStateVersion },
            "EQ Low", Range (-18.0f, 18.0f, 0.1f), 0.0f,
            juce::AudioParameterFloatAttributes().withStringFromValueFunction (dbFormat)));
        layout.add (std::make_unique<APF> (juce::ParameterID { eqMidGain, kStateVersion },
            "EQ Mid", Range (-18.0f, 18.0f, 0.1f), 0.0f,
            juce::AudioParameterFloatAttributes().withStringFromValueFunction (dbFormat)));
        layout.add (std::make_unique<APF> (juce::ParameterID { eqMidFreq, kStateVersion },
            "EQ Mid Freq", skewedRange (200.0f, 8000.0f, 1200.0f), 1000.0f,
            juce::AudioParameterFloatAttributes().withStringFromValueFunction (hzFormat)));
        layout.add (std::make_unique<APF> (juce::ParameterID { eqHighGain, kStateVersion },
            "EQ High", Range (-18.0f, 18.0f, 0.1f), 0.0f,
            juce::AudioParameterFloatAttributes().withStringFromValueFunction (dbFormat)));

        layout.add (std::make_unique<APB> (juce::ParameterID { compEnable, kStateVersion }, "Comp On", false));
        layout.add (std::make_unique<APF> (juce::ParameterID { compThresh, kStateVersion },
            "Comp Threshold", Range (-60.0f, 0.0f, 0.1f), -18.0f,
            juce::AudioParameterFloatAttributes().withStringFromValueFunction (dbFormat)));
        layout.add (std::make_unique<APF> (juce::ParameterID { compRatio, kStateVersion },
            "Comp Ratio", skewedRange (1.0f, 20.0f, 4.0f), 3.0f,
            juce::AudioParameterFloatAttributes().withStringFromValueFunction (
                [] (float v, int) { return juce::String (v, 1) + " : 1"; })));
        layout.add (std::make_unique<APF> (juce::ParameterID { compAttack, kStateVersion },
            "Comp Attack", skewedRange (0.1f, 100.0f, 10.0f), 10.0f,
            juce::AudioParameterFloatAttributes().withStringFromValueFunction (msFormat)));
        layout.add (std::make_unique<APF> (juce::ParameterID { compRelease, kStateVersion },
            "Comp Release", skewedRange (10.0f, 1000.0f, 150.0f), 150.0f,
            juce::AudioParameterFloatAttributes().withStringFromValueFunction (msFormat)));
    }

    // --------------------------------------------------------------- Global
    {
        using namespace global;
        layout.add (std::make_unique<APF> (juce::ParameterID { dryWet, kStateVersion },
            "Dry / Wet", Range (0.0f, 1.0f), 1.0f, pctAttr));
        layout.add (std::make_unique<APF> (juce::ParameterID { outputGain, kStateVersion },
            "Output Gain", Range (-24.0f, 12.0f, 0.1f), 0.0f,
            juce::AudioParameterFloatAttributes().withStringFromValueFunction (dbFormat)));
    }

    return layout;
}

} // namespace wavefront::params
