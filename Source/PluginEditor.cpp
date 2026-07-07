#include "PluginEditor.h"
#include "Params/ParameterIDs.h"

namespace wavefront
{

WavefrontAudioProcessorEditor::WavefrontAudioProcessorEditor (WavefrontAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p), pathEditor (p)
{
    addAndMakeVisible (pathEditor);

    // Sélecteur de mode (boutons radio).
    for (auto* b : { &pathModeBtn, &listenerModeBtn, &measureModeBtn })
    {
        b->setClickingTogglesState (true);
        b->setRadioGroupId (100);
        addAndMakeVisible (*b);
    }
    pathModeBtn.setToggleState (true, juce::dontSendNotification);

    pathModeBtn.onClick     = [this] { pathEditor.setMode (ui::PathEditorComponent::Mode::Path); };
    listenerModeBtn.onClick = [this] { pathEditor.setMode (ui::PathEditorComponent::Mode::Listener); };
    measureModeBtn.onClick  = [this] { pathEditor.setMode (ui::PathEditorComponent::Mode::Measure); };

    // Contrôles clés attachés à l'APVTS.
    using namespace params;
    addKnob (doppler::pathRate,      "Rate");
    addKnob (doppler::worldScale,    "Scale");
    addKnob (doppler::amount,        "Doppler");
    addKnob (doppler::distanceAtten, "Distance");
    addKnob (doppler::airAbsorption, "Air");
    addKnob (granular::mix,          "Grain Mix");
    addKnob (granular::pitch,        "Grain Pitch");
    addKnob (global::dryWet,         "Dry/Wet");
    addKnob (global::outputGain,     "Output");

    addAndMakeVisible (granularToggle);
    granularAttachment = std::make_unique<ButtonAttachment> (
        processorRef.getValueTree(), granular::enable, granularToggle);

    setResizable (true, true);
    setResizeLimits (760, 520, 2560, 1600);
    setSize (940, 640);
}

WavefrontAudioProcessorEditor::~WavefrontAudioProcessorEditor() = default;

WavefrontAudioProcessorEditor::Knob& WavefrontAudioProcessorEditor::addKnob (const juce::String& paramID,
                                                                             const juce::String& text)
{
    auto k = std::make_unique<Knob>();
    k->slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k->slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 68, 16);
    addAndMakeVisible (k->slider);

    k->label.setText (text, juce::dontSendNotification);
    k->label.setJustificationType (juce::Justification::centred);
    k->label.setFont (juce::Font (juce::FontOptions (12.0f)));
    addAndMakeVisible (k->label);

    k->attachment = std::make_unique<SliderAttachment> (processorRef.getValueTree(), paramID, k->slider);

    knobs.push_back (std::move (k));
    return *knobs.back();
}

void WavefrontAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff07070d));

    // Bandeau titre.
    g.setColour (juce::Colour (0xff9d4edd));
    g.setFont (juce::Font (juce::FontOptions (20.0f, juce::Font::bold)));
    g.drawText ("WAVEFRONT", getLocalBounds().removeFromTop (34).reduced (12, 0),
                juce::Justification::left, false);

    g.setColour (juce::Colours::grey);
    g.setFont (juce::Font (juce::FontOptions (11.0f)));
    g.drawText ("Doppler / warping", getLocalBounds().removeFromTop (34).reduced (12, 0),
                juce::Justification::right, false);
}

void WavefrontAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    area.removeFromTop (34); // bandeau

    // Bande de contrôles en bas.
    auto controls = area.removeFromBottom (110);

    // Bande de modes à gauche des contrôles.
    auto modeRow = area.removeFromBottom (30);
    auto modeArea = modeRow.removeFromLeft (330).reduced (4, 2);
    const int mw = modeArea.getWidth() / 3;
    pathModeBtn.setBounds     (modeArea.removeFromLeft (mw).reduced (2, 0));
    listenerModeBtn.setBounds (modeArea.removeFromLeft (mw).reduced (2, 0));
    measureModeBtn.setBounds  (modeArea.reduced (2, 0));
    granularToggle.setBounds  (modeRow.removeFromLeft (120).reduced (6, 2));

    // Path editor : reste de la zone.
    pathEditor.setBounds (area.reduced (10));

    // Disposition des rotatifs.
    const int n = (int) knobs.size();
    if (n > 0)
    {
        const int kw = controls.getWidth() / n;
        for (auto& k : knobs)
        {
            auto cell = controls.removeFromLeft (kw);
            k->label.setBounds (cell.removeFromTop (16));
            k->slider.setBounds (cell.reduced (4));
        }
    }
}

void WavefrontAudioProcessorEditor::updateModeButtons() {}

} // namespace wavefront
