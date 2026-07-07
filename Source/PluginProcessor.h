#pragma once

#include <JuceHeader.h>

namespace wavefront
{

/**
    Processeur audio principal de Wavefront.

    Phase 0 : squelette vide. Le signal passe en bypass (pass-through) le temps
    que les moteurs DSP (Doppler/warping, granulaire, matrice de modulation,
    effets post) soient implémentés dans les phases suivantes.
*/
class WavefrontAudioProcessor : public juce::AudioProcessor
{
public:
    WavefrontAudioProcessor();
    ~WavefrontAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WavefrontAudioProcessor)
};

} // namespace wavefront
