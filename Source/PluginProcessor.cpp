#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace wavefront
{

WavefrontAudioProcessor::WavefrontAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
}

WavefrontAudioProcessor::~WavefrontAudioProcessor() = default;

void WavefrontAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (sampleRate, samplesPerBlock);
    // Phase 0 : rien à préparer. Les moteurs DSP seront initialisés ici.
}

void WavefrontAudioProcessor::releaseResources()
{
    // Phase 0 : rien à libérer.
}

bool WavefrontAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Effet : on impose des layouts d'entrée et de sortie identiques,
    // en mono ou en stéréo.
    const auto& mainOutput = layouts.getMainOutputChannelSet();
    const auto& mainInput  = layouts.getMainInputChannelSet();

    if (mainOutput != juce::AudioChannelSet::mono()
        && mainOutput != juce::AudioChannelSet::stereo())
        return false;

    return mainInput == mainOutput;
}

void WavefrontAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                            juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);
    juce::ScopedNoDenormals noDenormals;

    // Vide les canaux de sortie qui n'ont pas d'entrée correspondante.
    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    // Phase 0 : pass-through (bypass). Le traitement Doppler/warping viendra ici.
}

juce::AudioProcessorEditor* WavefrontAudioProcessor::createEditor()
{
    return new WavefrontAudioProcessorEditor (*this);
}

void WavefrontAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::ignoreUnused (destData);
    // Phase 0 : pas d'état à sauvegarder (aucun paramètre pour l'instant).
}

void WavefrontAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::ignoreUnused (data, sizeInBytes);
    // Phase 0 : pas d'état à restaurer.
}

} // namespace wavefront

// Point d'entrée requis par JUCE pour instancier le plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new wavefront::WavefrontAudioProcessor();
}
