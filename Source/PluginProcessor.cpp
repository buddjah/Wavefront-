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
}

WavefrontAudioProcessor::~WavefrontAudioProcessor() = default;

void WavefrontAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    dopplerEngine.prepare (sampleRate, samplesPerBlock, getTotalNumInputChannels(), &pathModel);

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

void WavefrontAudioProcessor::pullParameters() noexcept
{
    dsp::DopplerEngine::Parameters dp;
    dp.pathRateHz    = pPathRate->load();
    dp.worldScaleM   = pWorldScale->load();
    dp.speedOfSound  = pSpeed->load();
    dp.amount        = pAmount->load();
    dp.distanceAtten = pDistAtten->load();
    dp.airAbsorption = pAir->load();
    dp.listenerX     = pListenerX->load();
    dp.listenerY     = pListenerY->load();
    dopplerEngine.setParameters (dp);

    dryWetSmoothed.setTargetValue (pDryWet->load());
    outputGainSmoothed.setTargetValue (juce::Decibels::decibelsToGain (pOutputGain->load()));
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

    pullParameters();

    // Copie du signal dry pour le mix ultérieur.
    dryBuffer.setSize (totalIn, numSamples, false, false, true);
    for (int ch = 0; ch < totalIn; ++ch)
        dryBuffer.copyFrom (ch, 0, buffer, ch, 0, numSamples);

    // Traitement Doppler (wet, en place).
    dopplerEngine.process (buffer);

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
