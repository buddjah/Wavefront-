#include "PluginEditor.h"

namespace wavefront
{

WavefrontAudioProcessorEditor::WavefrontAudioProcessorEditor (WavefrontAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    setResizable (true, true);
    setResizeLimits (480, 320, 2560, 1600);
    setSize (720, 480);
}

WavefrontAudioProcessorEditor::~WavefrontAudioProcessorEditor() = default;

void WavefrontAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Fond sombre — préfiguration de la future direction artistique néon.
    g.fillAll (juce::Colour (0xff0a0a12));

    g.setColour (juce::Colour (0xff9d4edd)); // violet "Path"
    g.setFont (juce::Font (juce::FontOptions (32.0f, juce::Font::bold)));
    g.drawText ("WAVEFRONT",
                getLocalBounds().reduced (20),
                juce::Justification::centred, false);

    g.setColour (juce::Colours::grey);
    g.setFont (juce::Font (juce::FontOptions (13.0f)));
    g.drawText ("Phase 0 — squelette VST3",
                getLocalBounds().removeFromBottom (40),
                juce::Justification::centred, false);
}

void WavefrontAudioProcessorEditor::resized()
{
    // Phase 0 : aucun composant enfant à positionner.
}

} // namespace wavefront
