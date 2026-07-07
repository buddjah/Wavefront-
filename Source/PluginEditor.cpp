#include "PluginEditor.h"
#include "Params/ParameterIDs.h"
#include "UI/LookAndFeel/WavefrontColours.h"

namespace wavefront
{

WavefrontAudioProcessorEditor::WavefrontAudioProcessorEditor (WavefrontAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p), pathEditor (p)
{
    setLookAndFeel (&lookAndFeel);
    addAndMakeVisible (pathEditor);

    // Sélecteur de mode (boutons radio) — accent violet (module Path).
    for (auto* b : { &pathModeBtn, &listenerModeBtn, &measureModeBtn })
    {
        b->setClickingTogglesState (true);
        b->setRadioGroupId (100);
        b->setColour (juce::TextButton::buttonOnColourId, ui::colours::path.withAlpha (0.25f));
        addAndMakeVisible (*b);
    }
    pathModeBtn.setToggleState (true, juce::dontSendNotification);

    pathModeBtn.onClick     = [this] { pathEditor.setMode (ui::PathEditorComponent::Mode::Path); };
    listenerModeBtn.onClick = [this] { pathEditor.setMode (ui::PathEditorComponent::Mode::Listener); };
    measureModeBtn.onClick  = [this] { pathEditor.setMode (ui::PathEditorComponent::Mode::Measure); };

    // Contrôles clés attachés à l'APVTS — accent par module.
    using namespace params;
    using namespace ui::colours;
    addKnob (doppler::pathRate,      "Rate",        path);
    addKnob (doppler::worldScale,    "Scale",       path);
    addKnob (doppler::amount,        "Doppler",     physics);
    addKnob (doppler::distanceAtten, "Distance",    physics);
    addKnob (doppler::airAbsorption, "Air",         physics);
    addKnob (params::sampler::gain,  "Smp Gain",    ui::colours::sampler);
    addKnob (params::sampler::pitch, "Smp Pitch",   ui::colours::sampler);
    addKnob (granular::mix,          "Grain Mix",   ui::colours::sampler);
    addKnob (granular::pitch,        "Grain Pitch", ui::colours::sampler);
    addKnob (global::dryWet,         "Dry/Wet",     effects);
    addKnob (global::outputGain,     "Output",      effects);

    granularToggle.setColour (juce::ToggleButton::tickColourId, ui::colours::sampler);
    addAndMakeVisible (granularToggle);
    granularAttachment = std::make_unique<ButtonAttachment> (
        processorRef.getValueTree(), granular::enable, granularToggle);

    // Presets.
    addAndMakeVisible (presetBox);
    presetBox.setTextWhenNothingSelected ("Presets");
    presetBox.onChange = [this] { handlePresetSelection(); };
    refreshPresetBox();

    addAndMakeVisible (savePresetBtn);
    savePresetBtn.onClick = [this] { saveUserPreset(); };

    // Sampler.
    samplerToggle.setColour (juce::ToggleButton::tickColourId, ui::colours::sampler);
    addAndMakeVisible (samplerToggle);
    samplerAttachment = std::make_unique<ButtonAttachment> (
        processorRef.getValueTree(), sampler::enable, samplerToggle);

    loopToggle.setColour (juce::ToggleButton::tickColourId, ui::colours::sampler);
    addAndMakeVisible (loopToggle);
    loopAttachment = std::make_unique<ButtonAttachment> (
        processorRef.getValueTree(), sampler::loop, loopToggle);

    addAndMakeVisible (sampleBox);
    sampleBox.setTextWhenNothingSelected ("Factory sample");
    sampleBox.onChange = [this] { handleSampleSelection(); };
    refreshSampleBox();

    addAndMakeVisible (loadSampleBtn);
    loadSampleBtn.onClick = [this] { chooseSampleFile(); };

    sampleNameLabel.setColour (juce::Label::textColourId, ui::colours::sampler);
    sampleNameLabel.setFont (juce::Font (juce::FontOptions (11.0f)));
    sampleNameLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (sampleNameLabel);
    updateSampleNameLabel();

    setResizable (true, true);
    setResizeLimits (760, 520, 2560, 1600);
    setSize (940, 640);
}

WavefrontAudioProcessorEditor::~WavefrontAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

WavefrontAudioProcessorEditor::Knob& WavefrontAudioProcessorEditor::addKnob (const juce::String& paramID,
                                                                             const juce::String& text,
                                                                             juce::Colour accent)
{
    auto k = std::make_unique<Knob>();
    k->slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k->slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 68, 16);
    k->slider.setColour (juce::Slider::rotarySliderFillColourId, accent);
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
    auto top = getLocalBounds().removeFromTop (38).reduced (12, 0);
    g.setColour (ui::colours::path);
    g.setFont (juce::Font (juce::FontOptions (20.0f, juce::Font::bold)));
    g.drawText ("WAVEFRONT", top, juce::Justification::left, false);

    g.setColour (ui::colours::dimText);
    g.setFont (juce::Font (juce::FontOptions (11.0f)));
    g.drawText ("Doppler / warping", top.withTrimmedLeft (150),
                juce::Justification::left, false);
}

void WavefrontAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    auto top = area.removeFromTop (38); // bandeau

    // Contrôles de presets à droite du bandeau.
    auto presetArea = top.removeFromRight (300).reduced (6, 7);
    savePresetBtn.setBounds (presetArea.removeFromRight (60).reduced (2, 0));
    presetBox.setBounds (presetArea.reduced (2, 0));

    // Bande de contrôles en bas.
    auto controls = area.removeFromBottom (110);

    // Bande de modes à gauche des contrôles.
    auto modeRow = area.removeFromBottom (30);
    auto modeArea = modeRow.removeFromLeft (330).reduced (4, 2);
    const int mw = modeArea.getWidth() / 3;
    pathModeBtn.setBounds     (modeArea.removeFromLeft (mw).reduced (2, 0));
    listenerModeBtn.setBounds (modeArea.removeFromLeft (mw).reduced (2, 0));
    measureModeBtn.setBounds  (modeArea.reduced (2, 0));
    granularToggle.setBounds  (modeRow.removeFromLeft (110).reduced (6, 2));

    // Bande sampler.
    auto samplerRow = area.removeFromBottom (30).reduced (4, 2);
    samplerToggle.setBounds (samplerRow.removeFromLeft (90));
    loopToggle.setBounds    (samplerRow.removeFromRight (70));
    loadSampleBtn.setBounds (samplerRow.removeFromRight (80).reduced (2, 1));
    sampleBox.setBounds     (samplerRow.removeFromLeft (180).reduced (2, 1));
    sampleNameLabel.setBounds (samplerRow.reduced (6, 0));

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

void WavefrontAudioProcessorEditor::refreshPresetBox()
{
    auto& pm = processorRef.getPresetManager();
    presetBox.clear (juce::dontSendNotification);

    presetBox.addSectionHeading ("Factory");
    for (int i = 0; i < pm.getNumFactoryPresets(); ++i)
        presetBox.addItem (pm.getFactoryPresetName (i), i + 1); // ids 1..N

    const auto users = pm.getUserPresetNames();
    if (! users.isEmpty())
    {
        presetBox.addSeparator();
        presetBox.addSectionHeading ("User");
        for (int i = 0; i < users.size(); ++i)
            presetBox.addItem (users[i], 1000 + i); // ids 1000+
    }
}

void WavefrontAudioProcessorEditor::handlePresetSelection()
{
    const int id = presetBox.getSelectedId();
    if (id <= 0) return;

    auto& pm = processorRef.getPresetManager();
    if (id < 1000)
        pm.loadFactoryPreset (id - 1);
    else
        pm.loadUserPreset (presetBox.getItemText (presetBox.getSelectedItemIndex()));

    repaint();
}

void WavefrontAudioProcessorEditor::saveUserPreset()
{
    saveDialog = std::make_unique<juce::AlertWindow> (
        "Save preset", "Nom du preset utilisateur :", juce::MessageBoxIconType::NoIcon);
    saveDialog->addTextEditor ("name", "My Preset");
    saveDialog->addButton ("Save",   1, juce::KeyPress (juce::KeyPress::returnKey));
    saveDialog->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    saveDialog->enterModalState (true, juce::ModalCallbackFunction::create (
        [this] (int result)
        {
            if (result == 1 && saveDialog != nullptr)
            {
                const auto name = saveDialog->getTextEditorContents ("name").trim();
                if (name.isNotEmpty())
                {
                    processorRef.getPresetManager().saveUserPreset (name);
                    refreshPresetBox();
                    presetBox.setText (name, juce::dontSendNotification);
                }
            }
            saveDialog.reset();
        }), false);
}

// ---- Sampler --------------------------------------------------------------

void WavefrontAudioProcessorEditor::refreshSampleBox()
{
    sampleBox.clear (juce::dontSendNotification);
    const auto names = processorRef.getFactorySampleNames();
    for (int i = 0; i < names.size(); ++i)
        sampleBox.addItem (names[i], i + 1);
}

void WavefrontAudioProcessorEditor::handleSampleSelection()
{
    const int idx = sampleBox.getSelectedId() - 1;
    if (idx >= 0)
    {
        processorRef.loadFactorySample (idx);
        updateSampleNameLabel();
    }
}

void WavefrontAudioProcessorEditor::chooseSampleFile()
{
    fileChooser = std::make_unique<juce::FileChooser> (
        "Charger un sample", juce::File(), "*.wav;*.aif;*.aiff;*.flac");

    const auto chooserFlags = juce::FileBrowserComponent::openMode
                            | juce::FileBrowserComponent::canSelectFiles;

    fileChooser->launchAsync (chooserFlags, [this] (const juce::FileChooser& fc)
    {
        const auto file = fc.getResult();
        if (file.existsAsFile())
        {
            processorRef.loadSampleFile (file);
            sampleBox.setSelectedId (0, juce::dontSendNotification);
            updateSampleNameLabel();
        }
    });
}

void WavefrontAudioProcessorEditor::updateSampleNameLabel()
{
    const auto name = processorRef.getLoadedSampleName();
    sampleNameLabel.setText (name.isEmpty() ? "(aucun sample)" : name, juce::dontSendNotification);
}

} // namespace wavefront
