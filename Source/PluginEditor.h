#pragma once

#include <JuceHeader.h>
#include <vector>
#include <memory>
#include "PluginProcessor.h"
#include "UI/PathEditor/PathEditorComponent.h"
#include "UI/LookAndFeel/WavefrontLookAndFeel.h"

namespace wavefront
{

/**
    Éditeur (UI) de Wavefront.

    Phase 3 : Path Editor 2D au centre, sélecteur de mode (Path/Listener/Measure)
    et bande de contrôles (rotatifs) attachés à l'APVTS pour les paramètres clés.
    La direction artistique néon complète arrive en Phase 4.
*/
class WavefrontAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit WavefrontAudioProcessorEditor (WavefrontAudioProcessor&);
    ~WavefrontAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    struct Knob
    {
        juce::Slider slider;
        juce::Label  label;
        std::unique_ptr<SliderAttachment> attachment;
    };

    Knob& addKnob (const juce::String& paramID, const juce::String& text, juce::Colour accent);
    void updateModeButtons();

    WavefrontAudioProcessor& processorRef;

    ui::WavefrontLookAndFeel lookAndFeel;
    ui::PathEditorComponent pathEditor;

    juce::TextButton pathModeBtn { "Path" }, listenerModeBtn { "Listener" }, measureModeBtn { "Measure" };

    juce::ToggleButton granularToggle { "Granular" };
    std::unique_ptr<ButtonAttachment> granularAttachment;

    std::vector<std::unique_ptr<Knob>> knobs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WavefrontAudioProcessorEditor)
};

} // namespace wavefront
