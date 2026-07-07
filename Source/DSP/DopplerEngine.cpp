#include "DopplerEngine.h"

namespace wavefront::dsp
{

void DopplerEngine::prepare (double sampleRate, int maxBlockSize, int numChannels, PathModel* pathModel)
{
    baseSampleRate = sampleRate;
    numCh = juce::jlimit (1, 2, numChannels);
    path = pathModel;

    oversampleFactor = 4;
    oversampling = std::make_unique<juce::dsp::Oversampling<float>> (
        (size_t) numCh, 2, // 2 étages → facteur 4
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        true, true);
    oversampling->initProcessing ((size_t) maxBlockSize);

    osSampleRate = baseSampleRate * oversampleFactor;

    // Dimensionnement de la ligne à retard : distance max ≈ 2·√2·worldScaleMax,
    // vitesse du son min = 100 m/s → retard max en secondes, converti à osRate.
    const float maxDistance = 2.0f * juce::MathConstants<float>::sqrt2 * 100.0f;
    const float maxDelaySec = maxDistance / 100.0f;
    const int   maxDelaySamples = (int) std::ceil (maxDelaySec * osSampleRate) + 8;

    for (auto& l : delayLines)
        l.prepare (maxDelaySamples);

    auto smoothTime = 0.05; // 50 ms
    for (auto* s : { &smPathRate, &smWorldScale, &smSpeed, &smAmount,
                     &smDistAtten, &smAir, &smListenerX, &smListenerY })
        s->reset (osSampleRate, smoothTime);

    smPathRate.setCurrentAndTargetValue (target.pathRateHz);
    smWorldScale.setCurrentAndTargetValue (target.worldScaleM);
    smSpeed.setCurrentAndTargetValue (target.speedOfSound);
    smAmount.setCurrentAndTargetValue (target.amount);
    smDistAtten.setCurrentAndTargetValue (target.distanceAtten);
    smAir.setCurrentAndTargetValue (target.airAbsorption);
    smListenerX.setCurrentAndTargetValue (target.listenerX);
    smListenerY.setCurrentAndTargetValue (target.listenerY);

    reset();
}

void DopplerEngine::reset()
{
    for (auto& l : delayLines) l.reset();
    airLpState = { 0.0f, 0.0f };
    phase = 0.0;
    meanDelay = 0.0f;
    prevDistanceM = 0.0f;
    if (oversampling) oversampling->reset();
}

int DopplerEngine::getLatencySamples() const noexcept
{
    if (oversampling == nullptr)
        return 0;
    return (int) oversampling->getLatencyInSamples();
}

float DopplerEngine::computeDelaySamples (float distanceMeters) const noexcept
{
    const float c = juce::jmax (1.0f, smSpeed.getCurrentValue());
    return distanceMeters / c * (float) osSampleRate;
}

void DopplerEngine::process (juce::AudioBuffer<float>& buffer) noexcept
{
    if (path == nullptr || oversampling == nullptr)
        return;

    smPathRate.setTargetValue (target.pathRateHz);
    smWorldScale.setTargetValue (target.worldScaleM);
    smSpeed.setTargetValue (target.speedOfSound);
    smAmount.setTargetValue (target.amount);
    smDistAtten.setTargetValue (target.distanceAtten);
    smAir.setTargetValue (target.airAbsorption);
    smListenerX.setTargetValue (target.listenerX);
    smListenerY.setTargetValue (target.listenerY);

    const int numChannels = juce::jmin (numCh, buffer.getNumChannels());

    juce::dsp::AudioBlock<float> block (buffer);
    auto osBlock = oversampling->processSamplesUp (block);

    const int osNumSamples = (int) osBlock.getNumSamples();

    // Coefficient de lissage du retard moyen : constante de temps ~ 0,3 s.
    const float meanCoef = 1.0f - std::exp (-1.0f / (0.3f * (float) osSampleRate));

    // Pré-calcul pour la phase (avance par échantillon OS).
    for (int n = 0; n < osNumSamples; ++n)
    {
        const float rate       = smPathRate.getNextValue();
        const float worldScale = smWorldScale.getNextValue();
        const float amount     = smAmount.getNextValue();
        const float distAtten  = smDistAtten.getNextValue();
        const float air        = smAir.getNextValue();
        const float lx         = smListenerX.getNextValue() * worldScale;
        const float ly         = smListenerY.getNextValue() * worldScale;
        smSpeed.getNextValue(); // garde le lissage synchrone (utilisé via getCurrentValue)

        // Avance de la phase de trajectoire.
        phase += (double) rate / osSampleRate;
        if (phase >= 1.0) phase -= std::floor (phase);

        const auto srcNorm = path->evaluate ((float) phase);
        const float sx = srcNorm.x * worldScale;
        const float sy = srcNorm.y * worldScale;

        const float dx = sx - lx;
        const float dy = sy - ly;
        float distance = std::sqrt (dx * dx + dy * dy);
        distance = juce::jmax (0.05f, distance);

        // Retard de propagation, séparé en composante moyenne (statique) et
        // composante Doppler (variation autour de la moyenne), pondérée par amount.
        const float instDelay = computeDelaySamples (distance);
        if (meanDelay <= 0.0f) meanDelay = instDelay;
        meanDelay += meanCoef * (instDelay - meanDelay);
        float delaySamples = meanDelay + amount * (instDelay - meanDelay);
        delaySamples = juce::jmax (1.0f, delaySamples);

        // Gain de distance : décroissant, borné (0,1].
        const float distGain = 1.0f / (1.0f + distAtten * (distance / juce::jmax (0.001f, worldScale)));

        // Absorption atmosphérique : passe-bas dont la coupure baisse avec la distance.
        // fc = 20 kHz à distance nulle → décroît exponentiellement selon air·distance.
        const float fc = juce::jlimit (200.0f, 20000.0f,
            20000.0f * std::exp (-air * distance / juce::jmax (1.0f, worldScale)));
        const float lpCoef = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * fc / (float) osSampleRate);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            float* data = osBlock.getChannelPointer ((size_t) ch);
            const float in = data[n];

            delayLines[(size_t) ch].pushSample (in);
            float wet = delayLines[(size_t) ch].readAt (delaySamples);

            // Absorption (one-pole).
            float& z = airLpState[(size_t) ch];
            z += lpCoef * (wet - z);
            wet = z;

            data[n] = wet * distGain;
        }

        // Dernier échantillon : publier l'état pour l'UI + vitesse radiale.
        if (n == osNumSamples - 1)
        {
            const float velocity = (distance - prevDistanceM);
            uiPhase.store    ((float) phase,               std::memory_order_relaxed);
            uiSrcX.store     (srcNorm.x,                    std::memory_order_relaxed);
            uiSrcY.store     (srcNorm.y,                    std::memory_order_relaxed);
            uiDistance.store (distance / juce::jmax (0.001f, worldScale), std::memory_order_relaxed);
            uiVelocity.store (juce::jlimit (-1.0f, 1.0f, velocity * 20.0f), std::memory_order_relaxed);
        }
        prevDistanceM = distance;
    }

    oversampling->processSamplesDown (block);
}

} // namespace wavefront::dsp
