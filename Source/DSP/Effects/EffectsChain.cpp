#include "EffectsChain.h"

namespace wavefront::dsp
{

void EffectsChain::prepare (double sr, int maxBlockSize, int numChannels)
{
    sampleRate = sr;
    numCh = juce::jlimit (1, 2, numChannels);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sr;
    spec.maximumBlockSize = (juce::uint32) maxBlockSize;
    spec.numChannels = (juce::uint32) numCh;

    eqLow.prepare (spec);
    eqMid.prepare (spec);
    eqHigh.prepare (spec);
    updateEqCoefficients();

    distOversampling = std::make_unique<juce::dsp::Oversampling<float>> (
        (size_t) numCh, 2,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true);
    distOversampling->initProcessing ((size_t) maxBlockSize);

    delayLine.prepare (spec);
    delayLine.setMaximumDelayInSamples ((int) std::ceil (2.05 * sr) + 4);
    delayTimeSmoothed.reset (sr, 0.05);
    delayTimeSmoothed.setCurrentAndTargetValue (params.dlyTimeMs * 0.001f * (float) sr);

    compressor.prepare (spec);

    reset();
}

void EffectsChain::reset()
{
    eqLow.reset(); eqMid.reset(); eqHigh.reset();
    if (distOversampling) distOversampling->reset();
    delayLine.reset();
    compressor.reset();
    tremPhase = 0.0;
}

int EffectsChain::getLatencySamples() const noexcept
{
    // La latence de l'oversampling de distortion n'est présente que lorsque la
    // distortion traite ; on la déclare toujours pour un routing stable.
    return distOversampling != nullptr ? (int) distOversampling->getLatencyInSamples() : 0;
}

void EffectsChain::updateEqCoefficients()
{
    const float lowFreq = 120.0f;
    const float highFreq = 6000.0f;

    *eqLow.state  = *Coeffs::makeLowShelf  (sampleRate, lowFreq, 0.707f,
                                            juce::Decibels::decibelsToGain (params.eqLowGain));
    *eqMid.state  = *Coeffs::makePeakFilter (sampleRate,
                                            juce::jlimit (200.0f, 8000.0f, params.eqMidFreq), 0.9f,
                                            juce::Decibels::decibelsToGain (params.eqMidGain));
    *eqHigh.state = *Coeffs::makeHighShelf (sampleRate, highFreq, 0.707f,
                                            juce::Decibels::decibelsToGain (params.eqHighGain));

    lastEqParams.eqLowGain = params.eqLowGain;
    lastEqParams.eqMidGain = params.eqMidGain;
    lastEqParams.eqMidFreq = params.eqMidFreq;
    lastEqParams.eqHighGain = params.eqHighGain;
}

void EffectsChain::processDistortion (juce::dsp::AudioBlock<float>& block) noexcept
{
    // Drive : gain d'entrée exponentiel avant le waveshaper tanh, compensé en sortie.
    const float drive = juce::jmap (params.distDrive, 0.0f, 1.0f, 1.0f, 40.0f);
    const float comp = 1.0f / std::tanh (drive); // compensation grossière du niveau
    const float mix = params.distMix;

    auto up = distOversampling->processSamplesUp (block);
    const int n = (int) up.getNumSamples();
    const int ch = (int) up.getNumChannels();

    for (int c = 0; c < ch; ++c)
    {
        float* d = up.getChannelPointer ((size_t) c);
        for (int i = 0; i < n; ++i)
            d[i] = std::tanh (d[i] * drive) * comp;
    }

    distOversampling->processSamplesDown (block);

    // Mix dry/wet appliqué après coup n'est pas possible ici (block déjà écrasé) :
    // le mix est géré en amont dans process() via une copie dry.
    juce::ignoreUnused (mix);
}

void EffectsChain::process (juce::AudioBuffer<float>& buffer) noexcept
{
    const int numSamples = buffer.getNumSamples();
    const int channels = juce::jmin (numCh, buffer.getNumChannels());

    juce::dsp::AudioBlock<float> block (buffer);
    auto ctxBlock = block.getSubsetChannelBlock (0, (size_t) channels);

    // ---- EQ ----
    if (params.eqOn)
    {
        const bool eqChanged =
               ! juce::approximatelyEqual (params.eqLowGain,  lastEqParams.eqLowGain)
            || ! juce::approximatelyEqual (params.eqMidGain,  lastEqParams.eqMidGain)
            || ! juce::approximatelyEqual (params.eqMidFreq,  lastEqParams.eqMidFreq)
            || ! juce::approximatelyEqual (params.eqHighGain, lastEqParams.eqHighGain);
        if (eqChanged)
            updateEqCoefficients();

        juce::dsp::ProcessContextReplacing<float> ctx (ctxBlock);
        eqLow.process (ctx);
        eqMid.process (ctx);
        eqHigh.process (ctx);
    }

    // ---- Distortion (oversamplée, avec mix dry/wet) ----
    if (params.distOn && distOversampling != nullptr)
    {
        juce::AudioBuffer<float> dry;
        dry.makeCopyOf (buffer, true);

        processDistortion (ctxBlock);

        const float mix = params.distMix;
        for (int c = 0; c < channels; ++c)
        {
            auto* wet = buffer.getWritePointer (c);
            auto* dr  = dry.getReadPointer (c);
            for (int i = 0; i < numSamples; ++i)
                wet[i] = dr[i] * (1.0f - mix) + wet[i] * mix;
        }
    }

    // ---- Compression ----
    if (params.compOn)
    {
        compressor.setThreshold (params.compThreshold);
        compressor.setRatio (juce::jmax (1.0f, params.compRatio));
        compressor.setAttack (params.compAttack);
        compressor.setRelease (params.compRelease);

        // Gain de compensation grossier (dépend du seuil et du ratio).
        const float makeup = juce::Decibels::decibelsToGain (
            -params.compThreshold * (1.0f - 1.0f / juce::jmax (1.0f, params.compRatio)) * 0.5f);

        juce::dsp::ProcessContextReplacing<float> ctx (ctxBlock);
        compressor.process (ctx);
        ctxBlock.multiplyBy (makeup);
    }

    // ---- Delay ----
    if (params.dlyOn)
    {
        delayTimeSmoothed.setTargetValue (params.dlyTimeMs * 0.001f * (float) sampleRate);
        const float fb = juce::jlimit (0.0f, 0.98f, params.dlyFeedback);
        const float mix = params.dlyMix;

        for (int i = 0; i < numSamples; ++i)
        {
            const float dTime = delayTimeSmoothed.getNextValue();
            for (int c = 0; c < channels; ++c)
            {
                const float in = buffer.getSample (c, i);
                const float delayed = delayLine.popSample (c, dTime, true);
                delayLine.pushSample (c, in + delayed * fb);
                buffer.setSample (c, i, in * (1.0f - mix) + delayed * mix);
            }
        }
    }

    // ---- Tremolo ----
    if (params.tremOn)
    {
        const double inc = (double) params.tremRate / sampleRate;
        const float depth = juce::jlimit (0.0f, 1.0f, params.tremDepth);

        for (int i = 0; i < numSamples; ++i)
        {
            const float lfo = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) tremPhase);
            const float gain = 1.0f - depth * lfo;
            for (int c = 0; c < channels; ++c)
                buffer.setSample (c, i, buffer.getSample (c, i) * gain);

            tremPhase += inc;
            if (tremPhase >= 1.0) tremPhase -= 1.0;
        }
    }
}

} // namespace wavefront::dsp
