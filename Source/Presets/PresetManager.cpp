#include "PresetManager.h"
#include "../Params/ParameterIDs.h"

namespace wavefront::presets
{

using namespace wavefront::params;

PresetManager::PresetManager (juce::AudioProcessorValueTreeState& state)
    : apvts (state)
{
    buildFactoryPresets();
}

// ---- Utilitaires ----------------------------------------------------------

void PresetManager::resetToDefaults()
{
    for (auto* p : apvts.processor.getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            rp->setValueNotifyingHost (rp->getDefaultValue());
}

void PresetManager::applyValue (const juce::String& paramID, float realValue)
{
    if (auto* p = apvts.getParameter (paramID))
        p->setValueNotifyingHost (p->convertTo0to1 (realValue));
}

// ---- Presets d'usine ------------------------------------------------------

juce::String PresetManager::getFactoryPresetName (int index) const
{
    if (juce::isPositiveAndBelow (index, (int) factory.size()))
        return factory[(size_t) index].name;
    return {};
}

void PresetManager::loadFactoryPreset (int index)
{
    if (! juce::isPositiveAndBelow (index, (int) factory.size()))
        return;

    const auto& preset = factory[(size_t) index];

    resetToDefaults();
    for (const auto& [id, value] : preset.values)
        applyValue (id, value);

    if (preset.factorySample.isNotEmpty() && onLoadFactorySample != nullptr)
        onLoadFactorySample (preset.factorySample);
}

void PresetManager::buildFactoryPresets()
{
    factory.clear();

    factory.push_back ({ "Init", {} }); // défauts

    factory.push_back ({ "Orbit Slow", {
        { doppler::pathRate, 0.3f }, { doppler::worldScale, 30.0f },
        { doppler::amount, 1.0f }, { doppler::airAbsorption, 0.5f },
        { doppler::distanceAtten, 0.4f }, { global::dryWet, 1.0f } } });

    factory.push_back ({ "Fast Flyby", {
        { doppler::pathRate, 2.4f }, { doppler::worldScale, 45.0f },
        { doppler::amount, 1.0f }, { doppler::distanceAtten, 0.7f },
        { doppler::airAbsorption, 0.6f }, { global::dryWet, 0.9f } } });

    factory.push_back ({ "Granular Cloud", {
        { doppler::pathRate, 0.5f }, { doppler::worldScale, 25.0f },
        { granular::enable, 1.0f }, { granular::mix, 0.7f },
        { granular::grainMs, 130.0f }, { granular::density, 55.0f },
        { granular::spray, 0.4f },
        { mod::lfoRate (0), 0.4f }, { mod::lfoDepth (0), 0.6f },
        { mod::slotSource (0), 1.0f /*LFO1*/ }, { mod::slotDest (0), 4.0f /*Grain Pitch*/ },
        { mod::slotDepth (0), 0.5f }, { global::dryWet, 1.0f } } });

    factory.push_back ({ "Warp Storm", {
        { doppler::pathRate, 3.2f }, { doppler::worldScale, 55.0f },
        { doppler::amount, 1.0f }, { doppler::distanceAtten, 0.5f },
        { fx::distEnable, 1.0f }, { fx::distDrive, 0.55f }, { fx::distMix, 0.8f },
        { fx::dlyEnable, 1.0f }, { fx::dlyTime, 220.0f }, { fx::dlyFeedback, 0.45f }, { fx::dlyMix, 0.35f },
        { mod::lfoRate (1), 0.25f }, { mod::lfoDepth (1), 0.8f },
        { mod::slotSource (1), 2.0f /*LFO2*/ }, { mod::slotDest (1), 2.0f /*World Scale*/ },
        { mod::slotDepth (1), 0.6f }, { global::dryWet, 1.0f } } });

    factory.push_back ({ "Ambient Wash", {
        { doppler::pathRate, 0.15f }, { doppler::worldScale, 40.0f },
        { doppler::amount, 0.8f }, { doppler::airAbsorption, 0.8f },
        { fx::dlyEnable, 1.0f }, { fx::dlyTime, 600.0f }, { fx::dlyFeedback, 0.5f }, { fx::dlyMix, 0.4f },
        { fx::eqEnable, 1.0f }, { fx::eqHighGain, -3.0f }, { fx::eqLowGain, 2.0f },
        { global::dryWet, 0.6f } } });

    // ---- Presets « instrument » : le sampler fournit la source ----

    factory.push_back ({ "Pad Voyager", {
        { sampler::enable, 1.0f }, { sampler::gain, -3.0f }, { sampler::pitch, 0.0f },
        { doppler::pathRate, 0.35f }, { doppler::worldScale, 30.0f }, { doppler::amount, 1.0f },
        { doppler::airAbsorption, 0.5f }, { global::dryWet, 1.0f } },
        "02_Pad_Chord.wav" });

    factory.push_back ({ "Bell Flyby", {
        { sampler::enable, 1.0f }, { sampler::gain, -4.0f },
        { doppler::pathRate, 1.8f }, { doppler::worldScale, 45.0f }, { doppler::amount, 1.0f },
        { doppler::distanceAtten, 0.6f }, { doppler::airAbsorption, 0.5f },
        { fx::dlyEnable, 1.0f }, { fx::dlyTime, 320.0f }, { fx::dlyFeedback, 0.4f }, { fx::dlyMix, 0.3f },
        { global::dryWet, 1.0f } },
        "05_Bell.wav" });

    factory.push_back ({ "Vowel Orbit", {
        { sampler::enable, 1.0f }, { sampler::gain, -5.0f },
        { doppler::pathRate, 0.6f }, { doppler::worldScale, 28.0f }, { doppler::amount, 1.0f },
        { granular::enable, 1.0f }, { granular::mix, 0.4f }, { granular::grainMs, 100.0f },
        { granular::density, 45.0f },
        { mod::lfoRate (0), 0.3f }, { mod::slotSource (0), 1.0f }, { mod::slotDest (0), 4.0f },
        { mod::slotDepth (0), 0.4f }, { global::dryWet, 1.0f } },
        "07_Vowel_Aah.wav" });

    factory.push_back ({ "Sub Pulse Warp", {
        { sampler::enable, 1.0f }, { sampler::gain, -2.0f },
        { doppler::pathRate, 0.5f }, { doppler::worldScale, 22.0f }, { doppler::amount, 0.9f },
        { fx::distEnable, 1.0f }, { fx::distDrive, 0.35f }, { fx::distMix, 0.6f },
        { fx::compEnable, 1.0f }, { fx::compThresh, -22.0f }, { fx::compRatio, 4.0f },
        { global::dryWet, 1.0f } },
        "06_Sub_Pulse.wav" });

    factory.push_back ({ "Metal Storm", {
        { sampler::enable, 1.0f }, { sampler::gain, -6.0f },
        { doppler::pathRate, 2.6f }, { doppler::worldScale, 55.0f }, { doppler::amount, 1.0f },
        { granular::enable, 1.0f }, { granular::mix, 0.6f }, { granular::grainMs, 60.0f },
        { granular::density, 70.0f }, { granular::spray, 0.5f },
        { mod::lfoRate (1), 0.2f }, { mod::slotSource (1), 2.0f }, { mod::slotDest (1), 2.0f },
        { mod::slotDepth (1), 0.6f },
        { fx::dlyEnable, 1.0f }, { fx::dlyTime, 260.0f }, { fx::dlyFeedback, 0.45f }, { fx::dlyMix, 0.35f },
        { global::dryWet, 1.0f } },
        "08_Metal_Drone.wav" });
}

// ---- Presets utilisateur --------------------------------------------------

juce::File PresetManager::getUserPresetDirectory()
{
    auto dir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                   .getChildFile ("Wavefront").getChildFile ("Presets");
    if (! dir.exists())
        dir.createDirectory();
    return dir;
}

juce::StringArray PresetManager::getUserPresetNames() const
{
    juce::StringArray names;
    for (auto& f : getUserPresetDirectory().findChildFiles (juce::File::findFiles, false, "*.xml"))
        names.add (f.getFileNameWithoutExtension());
    names.sortNatural();
    return names;
}

bool PresetManager::saveUserPreset (const juce::String& name)
{
    if (name.trim().isEmpty())
        return false;

    auto state = apvts.copyState();
    if (auto xml = state.createXml())
    {
        auto file = getUserPresetDirectory().getChildFile (name + ".xml");
        return xml->writeTo (file);
    }
    return false;
}

bool PresetManager::loadUserPreset (const juce::String& name)
{
    auto file = getUserPresetDirectory().getChildFile (name + ".xml");
    if (! file.existsAsFile())
        return false;

    if (auto xml = juce::XmlDocument::parse (file))
    {
        if (xml->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
            return true;
        }
    }
    return false;
}

} // namespace wavefront::presets
