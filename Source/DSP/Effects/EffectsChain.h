#pragma once

#include <JuceHeader.h>
#include <memory>

namespace wavefront::dsp
{

/**
    Chaîne d'effets post de Wavefront.

    Ordre : EQ -> Distortion (oversamplée ×4 séparément) -> Compression ->
    Delay -> Tremolo. Chaque étage a un bypass indépendant.

    Reste indépendante de la plateforme (JUCE DSP uniquement).
*/
class EffectsChain
{
public:
    struct Parameters
    {
        // Tremolo
        bool  tremOn = false;  float tremRate = 4.0f;  float tremDepth = 0.5f;
        // Delay
        bool  dlyOn = false;   float dlyTimeMs = 300.0f; float dlyFeedback = 0.35f; float dlyMix = 0.3f;
        // Distortion
        bool  distOn = false;  float distDrive = 0.3f;  float distMix = 1.0f;
        // EQ
        bool  eqOn = false;    float eqLowGain = 0.0f;  float eqMidGain = 0.0f;
        float eqMidFreq = 1000.0f; float eqHighGain = 0.0f;
        // Compression
        bool  compOn = false;  float compThreshold = -18.0f; float compRatio = 3.0f;
        float compAttack = 10.0f; float compRelease = 150.0f;
    };

    void prepare (double sampleRate, int maxBlockSize, int numChannels);
    void reset();

    void setParameters (const Parameters& p) noexcept { params = p; }
    void process (juce::AudioBuffer<float>& buffer) noexcept;

    /** Latence introduite (oversampling de la distortion). */
    int getLatencySamples() const noexcept;

private:
    void updateEqCoefficients();
    void processDistortion (juce::dsp::AudioBlock<float>& block) noexcept;

    double sampleRate = 44100.0;
    int    numCh = 2;

    Parameters params;
    Parameters lastEqParams; // pour ne recalculer les coeffs qu'au besoin

    // EQ : 3 bandes (low shelf, mid peak, high shelf), stéréo.
    using Filter = juce::dsp::IIR::Filter<float>;
    using Coeffs = juce::dsp::IIR::Coefficients<float>;
    using Duplicator = juce::dsp::ProcessorDuplicator<Filter, Coeffs>;
    Duplicator eqLow, eqMid, eqHigh;

    // Distortion oversamplée.
    std::unique_ptr<juce::dsp::Oversampling<float>> distOversampling;

    // Delay stéréo.
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayLine { 96000 };
    juce::SmoothedValue<float> delayTimeSmoothed;

    // Compression.
    juce::dsp::Compressor<float> compressor;

    // Tremolo.
    double tremPhase = 0.0;
};

} // namespace wavefront::dsp
