#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

namespace wavefront
{

/**
    Éditeur (UI) de Wavefront.

    Phase 0 : fenêtre vide affichant simplement le nom du plugin. La direction
    artistique néon et le Path Editor 2D seront implémentés dans les phases 3 et 4.
*/
class WavefrontAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit WavefrontAudioProcessorEditor (WavefrontAudioProcessor&);
    ~WavefrontAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    WavefrontAudioProcessor& processorRef;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WavefrontAudioProcessorEditor)
};

} // namespace wavefront
