#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Params/ParameterIDs.h"

namespace wavefront
{

WavefrontAudioProcessor::WavefrontAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "WAVEFRONT_STATE", params::createParameterLayout())
{
    using namespace params;
    pPathRate   = apvts.getRawParameterValue (doppler::pathRate);
    pWorldScale = apvts.getRawParameterValue (doppler::worldScale);
    pSpeed      = apvts.getRawParameterValue (doppler::speedOfSound);
    pAmount     = apvts.getRawParameterValue (doppler::amount);
    pDistAtten  = apvts.getRawParameterValue (doppler::distanceAtten);
    pAir        = apvts.getRawParameterValue (doppler::airAbsorption);
    pListenerX  = apvts.getRawParameterValue (doppler::listenerX);
    pListenerY  = apvts.getRawParameterValue (doppler::listenerY);
    pDryWet     = apvts.getRawParameterValue (global::dryWet);
    pOutputGain = apvts.getRawParameterValue (global::outputGain);

    pGrEnable   = apvts.getRawParameterValue (granular::enable);
    pGrMix      = apvts.getRawParameterValue (granular::mix);
    pGrSize     = apvts.getRawParameterValue (granular::grainMs);
    pGrDensity  = apvts.getRawParameterValue (granular::density);
    pGrSpray    = apvts.getRawParameterValue (granular::spray);
    pGrPitch    = apvts.getRawParameterValue (granular::pitch);

    for (int i = 0; i < dsp::ModMatrix::kNumLfos; ++i)
    {
        pLfoRate[(size_t) i]    = apvts.getRawParameterValue (mod::lfoRate (i));
        pLfoDepth[(size_t) i]   = apvts.getRawParameterValue (mod::lfoDepth (i));
        pLfoShape[(size_t) i]   = apvts.getRawParameterValue (mod::lfoShape (i));
        pLfoBipolar[(size_t) i] = apvts.getRawParameterValue (mod::lfoBipolar (i));
    }
    for (int i = 0; i < dsp::ModMatrix::kNumSlots; ++i)
    {
        pSlotSource[(size_t) i] = apvts.getRawParameterValue (mod::slotSource (i));
        pSlotDest[(size_t) i]   = apvts.getRawParameterValue (mod::slotDest (i));
        pSlotDepth[(size_t) i]  = apvts.getRawParameterValue (mod::slotDepth (i));
        pSlotSmooth[(size_t) i] = apvts.getRawParameterValue (mod::slotSmooth (i));
    }
}

WavefrontAudioProcessor::~WavefrontAudioProcessor() = default;

void WavefrontAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    dopplerEngine.prepare (sampleRate, samplesPerBlock, getTotalNumInputChannels(), &pathModel);
    granularEngine.prepare (sampleRate, getTotalNumInputChannels());
    modMatrix.prepare (sampleRate);
    effectsChain.prepare (sampleRate, samplesPerBlock, getTotalNumInputChannels());

    dryBuffer.setSize (getTotalNumInputChannels(), samplesPerBlock);

    dryWetSmoothed.reset (sampleRate, 0.02);
    outputGainSmoothed.reset (sampleRate, 0.02);
    dryWetSmoothed.setCurrentAndTargetValue (pDryWet->load());
    outputGainSmoothed.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (pOutputGain->load()));

    setLatencySamples (dopplerEngine.getLatencySamples());
}

void WavefrontAudioProcessor::releaseResources()
{
    dopplerEngine.reset();
    granularEngine.reset();
    modMatrix.reset();
    effectsChain.reset();
}

bool WavefrontAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& mainOutput = layouts.getMainOutputChannelSet();
    const auto& mainInput  = layouts.getMainInputChannelSet();

    if (mainOutput != juce::AudioChannelSet::mono()
        && mainOutput != juce::AudioChannelSet::stereo())
        return false;

    return mainInput == mainOutput;
}

void WavefrontAudioProcessor::pullParameters (int numSamples) noexcept
{
    using namespace dsp;

    // --- 1) Configure LFOs + slots depuis l'APVTS ---
    for (int i = 0; i < ModMatrix::kNumLfos; ++i)
    {
        ModMatrix::LfoSettings s;
        s.rate    = pLfoRate[(size_t) i]->load();
        s.depth   = pLfoDepth[(size_t) i]->load();
        s.shape   = (LFO::Shape) (int) pLfoShape[(size_t) i]->load();
        s.bipolar = pLfoBipolar[(size_t) i]->load() > 0.5f;
        modMatrix.setLfo (i, s);
    }
    for (int i = 0; i < ModMatrix::kNumSlots; ++i)
    {
        ModMatrix::Slot s;
        s.source = (ModSource) (int) pSlotSource[(size_t) i]->load();
        s.dest   = (ModDest)   (int) pSlotDest[(size_t) i]->load();
        s.depth  = pSlotDepth[(size_t) i]->load();
        s.smooth = pSlotSmooth[(size_t) i]->load();
        modMatrix.setSlot (i, s);
    }

    // --- 2) Calcule la modulation (sources trajectoire = bloc précédent) ---
    ModMatrix::PathSources ps;
    ps.distance = dopplerEngine.getNormDistance();
    ps.velocity = dopplerEngine.getRadialVelocity();
    ps.x        = dopplerEngine.getSourceX();
    ps.y        = dopplerEngine.getSourceY();
    modMatrix.process (numSamples, ps);

    auto m = [this] (ModDest d) { return modMatrix.getModulation (d); };

    // --- 3) Applique base + modulation, avec bornage ---
    DopplerEngine::Parameters dp;
    dp.pathRateHz    = pPathRate->load();
    dp.worldScaleM   = juce::jlimit (1.0f, 100.0f,
                                     pWorldScale->load() * (1.0f + 0.5f * m (ModDest::WorldScale)));
    dp.speedOfSound  = pSpeed->load();
    dp.amount        = juce::jlimit (0.0f, 1.0f, pAmount->load() + 0.5f * m (ModDest::DopplerAmount));
    dp.distanceAtten = pDistAtten->load();
    dp.airAbsorption = pAir->load();
    dp.listenerX     = pListenerX->load();
    dp.listenerY     = pListenerY->load();
    dopplerEngine.setParameters (dp);

    GranularEngine::Parameters gp;
    gp.enabled = pGrEnable->load() > 0.5f;
    gp.mix     = pGrMix->load();
    gp.grainMs = juce::jlimit (5.0f, 500.0f, pGrSize->load() * (1.0f + 0.5f * m (ModDest::GrainSize)));
    gp.density = juce::jlimit (1.0f, 200.0f, pGrDensity->load() * (1.0f + 0.5f * m (ModDest::GrainDensity)));
    gp.spray   = pGrSpray->load();
    gp.pitchSt = juce::jlimit (-24.0f, 24.0f, pGrPitch->load() + 12.0f * m (ModDest::GrainPitch));
    granularEngine.setParameters (gp);

    // --- Effets post (base + modulation pour trem depth / delay time / drive) ---
    using namespace params::fx;
    auto gv = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };

    EffectsChain::Parameters ep;
    ep.tremOn        = gv (tremEnable) > 0.5f;
    ep.tremRate      = gv (tremRate);
    ep.tremDepth     = juce::jlimit (0.0f, 1.0f, gv (tremDepth) + 0.5f * m (ModDest::TremDepth));
    ep.dlyOn         = gv (dlyEnable) > 0.5f;
    ep.dlyTimeMs     = juce::jlimit (1.0f, 2000.0f, gv (dlyTime) * (1.0f + 0.5f * m (ModDest::DelayTime)));
    ep.dlyFeedback   = gv (dlyFeedback);
    ep.dlyMix        = gv (dlyMix);
    ep.distOn        = gv (distEnable) > 0.5f;
    ep.distDrive     = juce::jlimit (0.0f, 1.0f, gv (distDrive) + 0.5f * m (ModDest::DistDrive));
    ep.distMix       = gv (distMix);
    ep.eqOn          = gv (eqEnable) > 0.5f;
    ep.eqLowGain     = gv (eqLowGain);
    ep.eqMidGain     = gv (eqMidGain);
    ep.eqMidFreq     = gv (eqMidFreq);
    ep.eqHighGain    = gv (eqHighGain);
    ep.compOn        = gv (compEnable) > 0.5f;
    ep.compThreshold = gv (compThresh);
    ep.compRatio     = gv (compRatio);
    ep.compAttack    = gv (compAttack);
    ep.compRelease   = gv (compRelease);
    effectsChain.setParameters (ep);

    const float outGainDb = pOutputGain->load() + 6.0f * m (ModDest::OutputGain);
    dryWetSmoothed.setTargetValue (pDryWet->load());
    outputGainSmoothed.setTargetValue (juce::Decibels::decibelsToGain (outGainDb));
}

void WavefrontAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                            juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);
    juce::ScopedNoDenormals noDenormals;

    const int totalIn  = getTotalNumInputChannels();
    const int totalOut = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();

    for (int ch = totalIn; ch < totalOut; ++ch)
        buffer.clear (ch, 0, numSamples);

    pullParameters (numSamples);

    // Copie du signal dry pour le mix ultérieur.
    dryBuffer.setSize (totalIn, numSamples, false, false, true);
    for (int ch = 0; ch < totalIn; ++ch)
        dryBuffer.copyFrom (ch, 0, buffer, ch, 0, numSamples);

    // Chaîne « wet » (en place) : Doppler -> Granulaire -> Effets post.
    dopplerEngine.process (buffer);
    granularEngine.process (buffer, dopplerEngine.getPhase());
    effectsChain.process (buffer);

    // Mix dry/wet + gain de sortie, échantillon par échantillon (anti-zipper).
    for (int i = 0; i < numSamples; ++i)
    {
        const float mix  = dryWetSmoothed.getNextValue();
        const float gain = outputGainSmoothed.getNextValue();

        for (int ch = 0; ch < totalIn; ++ch)
        {
            const float dry = dryBuffer.getSample (ch, i);
            const float wet = buffer.getSample (ch, i);
            buffer.setSample (ch, i, (dry * (1.0f - mix) + wet * mix) * gain);
        }
    }
}

juce::AudioProcessorEditor* WavefrontAudioProcessor::createEditor()
{
    return new WavefrontAudioProcessorEditor (*this);
}

void WavefrontAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); state.isValid())
    {
        std::unique_ptr<juce::XmlElement> xml (state.createXml());
        copyXmlToBinary (*xml, destData);
    }
}

void WavefrontAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

} // namespace wavefront

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new wavefront::WavefrontAudioProcessor();
}
