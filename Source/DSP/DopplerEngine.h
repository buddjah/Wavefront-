#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>
#include "HQFractionalDelay.h"
#include "PathModel.h"

namespace wavefront::dsp
{

/**
    Moteur Doppler / warping haute qualité.

    Principe physique : la source parcourt la trajectoire (PathModel) autour de
    l'auditeur. À chaque échantillon on calcule la distance source→auditeur, d'où
    un retard de propagation (d / c). Un retard *variable* appliqué à une ligne à
    retard fractionnaire produit naturellement le décalage de hauteur (Doppler) :
    quand la source s'approche, le retard diminue, la lecture accélère, la hauteur
    monte — et inversement.

    S'y ajoutent :
      - l'atténuation liée à la distance,
      - un filtre passe-bas d'absorption atmosphérique (les aigus s'atténuent avec
        la distance),
      - un oversampling ×4 (juce::dsp::Oversampling) pour limiter le repliement dû
        aux fortes modulations de hauteur,
      - un lissage anti-zipper des paramètres.

    Le moteur publie sa phase de lecture et la position/distance courantes pour
    l'UI (playhead animé), en lock-free (atomiques).
*/
class DopplerEngine
{
public:
    struct Parameters
    {
        float pathRateHz     = 0.5f;
        float worldScaleM    = 20.0f;
        float speedOfSound   = 343.0f;
        float amount         = 1.0f;
        float distanceAtten  = 0.5f;
        float airAbsorption  = 0.4f;
        float listenerX      = 0.0f;
        float listenerY      = 0.0f;
    };

    DopplerEngine() = default;

    void prepare (double sampleRate, int maxBlockSize, int numChannels, PathModel* pathModel);
    void reset();

    /** À appeler une fois par bloc avec les valeurs de paramètres cibles. */
    void setParameters (const Parameters& p) noexcept { target = p; }

    /** Traite le bloc en place (produit le signal « wet »). */
    void process (juce::AudioBuffer<float>& buffer) noexcept;

    int getLatencySamples() const noexcept;

    // ---- Retour pour l'UI (lock-free) ----
    float getPhase() const noexcept        { return uiPhase.load (std::memory_order_relaxed); }
    float getSourceX() const noexcept      { return uiSrcX.load (std::memory_order_relaxed); }
    float getSourceY() const noexcept      { return uiSrcY.load (std::memory_order_relaxed); }
    float getNormDistance() const noexcept { return uiDistance.load (std::memory_order_relaxed); }
    /** Vitesse radiale normalisée (source de modulation dérivée de la trajectoire). */
    float getRadialVelocity() const noexcept { return uiVelocity.load (std::memory_order_relaxed); }

private:
    float computeDelaySamples (float distanceMeters) const noexcept;

    PathModel* path = nullptr;

    double baseSampleRate = 44100.0;
    double osSampleRate   = 176400.0;
    int    oversampleFactor = 4;
    int    numCh = 2;

    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;

    std::array<HQFractionalDelay, 2> delayLines;
    std::array<float, 2> airLpState { 0.0f, 0.0f };

    // Phase de lecture de la trajectoire (avance au taux audio, à osSampleRate).
    double phase = 0.0;

    // Suivi lissé du retard moyen (pour séparer composante statique / Doppler).
    float meanDelay = 0.0f;
    float prevDistanceM = 0.0f;

    // Lissage anti-zipper.
    juce::SmoothedValue<float> smPathRate, smWorldScale, smSpeed, smAmount,
                               smDistAtten, smAir, smListenerX, smListenerY;

    Parameters target;

    // Retour UI.
    std::atomic<float> uiPhase { 0.0f }, uiSrcX { 0.0f }, uiSrcY { 0.0f },
                       uiDistance { 0.0f }, uiVelocity { 0.0f };
};

} // namespace wavefront::dsp
