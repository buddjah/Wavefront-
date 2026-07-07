#pragma once

#include <JuceHeader.h>

/**
    Identifiants (String IDs) et versions des paramètres exposés par Wavefront
    via l'AudioProcessorValueTreeState (APVTS).

    Les IDs sont regroupés par module. Ils sont volontairement stables : ne pas
    les renommer une fois des presets sauvegardés (compatibilité d'état).
*/
namespace wavefront::params
{

// Version du schéma de paramètres (à incrémenter en cas de migration).
inline constexpr int kStateVersion = 1;

// ---- Doppler / warping (Phase 1) ----
namespace doppler
{
    inline constexpr auto pathRate      = "doppler.pathRate";      // Hz, vitesse d'orbite du playhead
    inline constexpr auto worldScale    = "doppler.worldScale";    // mètres, taille de la trajectoire
    inline constexpr auto speedOfSound  = "doppler.speedOfSound";  // m/s
    inline constexpr auto amount        = "doppler.amount";        // 0..1, intensité de l'effet Doppler
    inline constexpr auto distanceAtten = "doppler.distanceAtten"; // 0..1, atténuation liée à la distance
    inline constexpr auto airAbsorption = "doppler.airAbsorption"; // 0..1, absorption atmosphérique HF
    inline constexpr auto listenerX     = "doppler.listenerX";     // -1..1, position X de l'auditeur
    inline constexpr auto listenerY     = "doppler.listenerY";     // -1..1, position Y de l'auditeur
}

// ---- Granulaire (Phase 2) ----
namespace granular
{
    inline constexpr auto enable   = "granular.enable";
    inline constexpr auto mix      = "granular.mix";      // 0..1
    inline constexpr auto grainMs  = "granular.grainMs";  // ms, taille de grain
    inline constexpr auto density  = "granular.density";  // grains/s
    inline constexpr auto spray    = "granular.spray";    // 0..1, dispersion aléatoire de position
    inline constexpr auto pitch    = "granular.pitch";    // demi-tons, transposition des grains
}

// ---- Sampler (Phase 5+) ----
namespace sampler
{
    inline constexpr auto enable = "sampler.enable";
    inline constexpr auto gain   = "sampler.gain";   // dB
    inline constexpr auto loop   = "sampler.loop";
    inline constexpr auto pitch  = "sampler.pitch";  // demi-tons
}

// ---- LFOs / Matrice de modulation (Phase 2) ----
namespace mod
{
    inline constexpr int  kNumLfos = 4;
    inline constexpr int  kNumSlots = 8; // connexions de la matrice

    // Motif : lfoN.<param>
    inline auto lfoRate     (int n) { return "lfo" + juce::String (n) + ".rate";     }
    inline auto lfoDepth    (int n) { return "lfo" + juce::String (n) + ".depth";    }
    inline auto lfoShape    (int n) { return "lfo" + juce::String (n) + ".shape";    }
    inline auto lfoBipolar  (int n) { return "lfo" + juce::String (n) + ".bipolar";  }

    // Motif : slotN.<param>
    inline auto slotSource  (int n) { return "slot" + juce::String (n) + ".source"; }
    inline auto slotDest    (int n) { return "slot" + juce::String (n) + ".dest";   }
    inline auto slotDepth   (int n) { return "slot" + juce::String (n) + ".depth";  }
    inline auto slotSmooth  (int n) { return "slot" + juce::String (n) + ".smooth"; }
}

// ---- Effets post (Phase 5) ----
namespace fx
{
    inline constexpr auto tremEnable = "fx.trem.enable";
    inline constexpr auto tremRate   = "fx.trem.rate";
    inline constexpr auto tremDepth  = "fx.trem.depth";

    inline constexpr auto dlyEnable  = "fx.dly.enable";
    inline constexpr auto dlyTime    = "fx.dly.time";
    inline constexpr auto dlyFeedback= "fx.dly.feedback";
    inline constexpr auto dlyMix     = "fx.dly.mix";

    inline constexpr auto distEnable = "fx.dist.enable";
    inline constexpr auto distDrive  = "fx.dist.drive";
    inline constexpr auto distMix    = "fx.dist.mix";

    inline constexpr auto eqEnable   = "fx.eq.enable";
    inline constexpr auto eqLowGain  = "fx.eq.lowGain";
    inline constexpr auto eqMidGain  = "fx.eq.midGain";
    inline constexpr auto eqMidFreq  = "fx.eq.midFreq";
    inline constexpr auto eqHighGain = "fx.eq.highGain";

    inline constexpr auto compEnable = "fx.comp.enable";
    inline constexpr auto compThresh = "fx.comp.threshold";
    inline constexpr auto compRatio  = "fx.comp.ratio";
    inline constexpr auto compAttack = "fx.comp.attack";
    inline constexpr auto compRelease= "fx.comp.release";
}

// ---- Global ----
namespace global
{
    inline constexpr auto dryWet     = "global.dryWet";     // 0..1
    inline constexpr auto outputGain = "global.outputGain"; // dB
}

} // namespace wavefront::params
