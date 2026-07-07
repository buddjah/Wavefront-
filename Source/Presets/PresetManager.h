#pragma once

#include <JuceHeader.h>
#include <vector>

namespace wavefront::presets
{

/**
    Gestion des presets.

    - presets d'usine définis en code (liste de {paramID, valeur réelle}) ;
    - presets utilisateur sauvegardés/chargés en XML dans le dossier
      Wavefront/Presets des données application ;
    - chaque chargement remet d'abord tous les paramètres à leur défaut avant
      d'appliquer les surcharges -> résultat déterministe.
*/
class PresetManager
{
public:
    explicit PresetManager (juce::AudioProcessorValueTreeState& state);

    // ---- Presets d'usine ----
    int getNumFactoryPresets() const noexcept { return (int) factory.size(); }
    juce::String getFactoryPresetName (int index) const;
    void loadFactoryPreset (int index);

    // ---- Presets utilisateur (fichiers) ----
    juce::StringArray getUserPresetNames() const;
    bool saveUserPreset (const juce::String& name);
    bool loadUserPreset (const juce::String& name);

    static juce::File getUserPresetDirectory();

private:
    struct FactoryPreset
    {
        juce::String name;
        std::vector<std::pair<juce::String, float>> values; // {paramID, valeur réelle}
    };

    void resetToDefaults();
    void applyValue (const juce::String& paramID, float realValue);
    void buildFactoryPresets();

    juce::AudioProcessorValueTreeState& apvts;
    std::vector<FactoryPreset> factory;
};

} // namespace wavefront::presets
