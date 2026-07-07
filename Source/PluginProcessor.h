#pragma once

#include <JuceHeader.h>
#include <array>
#include "Params/Parameters.h"
#include "DSP/PathModel.h"
#include "DSP/DopplerEngine.h"
#include "DSP/Granular/GranularEngine.h"
#include "DSP/Modulation/ModMatrix.h"
#include "DSP/Effects/EffectsChain.h"
#include "DSP/Sampler/SamplerEngine.h"
#include "Presets/PresetManager.h"

namespace wavefront
{

/**
    Processeur audio principal de Wavefront.

    Phase 1 : moteur Doppler/warping HQ opérationnel (PathModel + DopplerEngine),
    piloté par l'APVTS, avec mix dry/wet et gain de sortie.
*/
class WavefrontAudioProcessor : public juce::AudioProcessor
{
public:
    WavefrontAudioProcessor();
    ~WavefrontAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    using juce::AudioProcessor::processBlock; // évite le masquage de la surcharge double
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

    // ---- Accès pour l'éditeur / UI ----
    juce::AudioProcessorValueTreeState& getValueTree() noexcept { return apvts; }
    dsp::PathModel&     getPathModel()     noexcept { return pathModel; }
    dsp::DopplerEngine& getDopplerEngine() noexcept { return dopplerEngine; }
    presets::PresetManager& getPresetManager() noexcept { return presetManager; }
    dsp::SamplerEngine& getSamplerEngine() noexcept { return samplerEngine; }

    // ---- Chargement des samples (thread message) ----
    juce::StringArray getFactorySampleNames() const;
    void loadFactorySample (int index);
    void loadSampleFile (const juce::File& file);
    juce::String getLoadedSampleName() const { return samplerEngine.getLoadedName(); }

private:
    void pullParameters (int numSamples) noexcept;

    juce::AudioProcessorValueTreeState apvts;
    presets::PresetManager presetManager { apvts };

    dsp::PathModel      pathModel;
    dsp::DopplerEngine  dopplerEngine;
    dsp::GranularEngine granularEngine;
    dsp::ModMatrix      modMatrix;
    dsp::EffectsChain   effectsChain;
    dsp::SamplerEngine  samplerEngine;
    juce::AudioFormatManager formatManager;

    dsp::SamplerEngine::Sample::Ptr makeSampleFromReader (juce::AudioFormatReader*, const juce::String& name);
    void reloadSamplerSource();

    juce::AudioBuffer<float> dryBuffer;
    juce::SmoothedValue<float> dryWetSmoothed, outputGainSmoothed;

    // Pointeurs bruts vers les paramètres (accès rapide, sans lookup par ID).
    std::atomic<float>* pPathRate = nullptr;
    std::atomic<float>* pWorldScale = nullptr;
    std::atomic<float>* pSpeed = nullptr;
    std::atomic<float>* pAmount = nullptr;
    std::atomic<float>* pDistAtten = nullptr;
    std::atomic<float>* pAir = nullptr;
    std::atomic<float>* pListenerX = nullptr;
    std::atomic<float>* pListenerY = nullptr;
    std::atomic<float>* pDryWet = nullptr;
    std::atomic<float>* pOutputGain = nullptr;

    // Sampler.
    std::atomic<float>* pSmpEnable = nullptr;
    std::atomic<float>* pSmpGain = nullptr;
    std::atomic<float>* pSmpLoop = nullptr;
    std::atomic<float>* pSmpPitch = nullptr;

    // Granulaire.
    std::atomic<float>* pGrEnable = nullptr;
    std::atomic<float>* pGrMix = nullptr;
    std::atomic<float>* pGrSize = nullptr;
    std::atomic<float>* pGrDensity = nullptr;
    std::atomic<float>* pGrSpray = nullptr;
    std::atomic<float>* pGrPitch = nullptr;

    // LFOs & matrice (tableaux de pointeurs bruts).
    std::array<std::atomic<float>*, dsp::ModMatrix::kNumLfos>  pLfoRate {};
    std::array<std::atomic<float>*, dsp::ModMatrix::kNumLfos>  pLfoDepth {};
    std::array<std::atomic<float>*, dsp::ModMatrix::kNumLfos>  pLfoShape {};
    std::array<std::atomic<float>*, dsp::ModMatrix::kNumLfos>  pLfoBipolar {};
    std::array<std::atomic<float>*, dsp::ModMatrix::kNumSlots> pSlotSource {};
    std::array<std::atomic<float>*, dsp::ModMatrix::kNumSlots> pSlotDest {};
    std::array<std::atomic<float>*, dsp::ModMatrix::kNumSlots> pSlotDepth {};
    std::array<std::atomic<float>*, dsp::ModMatrix::kNumSlots> pSlotSmooth {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WavefrontAudioProcessor)
};

} // namespace wavefront
