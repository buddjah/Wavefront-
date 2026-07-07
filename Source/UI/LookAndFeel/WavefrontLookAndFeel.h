#pragma once

#include <JuceHeader.h>
#include "WavefrontColours.h"

namespace wavefront::ui
{

/**
    LookAndFeel néon sombre de Wavefront.

    - fonds sombres, texte clair ;
    - rotatifs à arc néon avec glow (double tracé translucide + trait net) ;
    - l'accent de chaque contrôle est piloté par sa couleur
      `rotarySliderFillColourId` (mapping couleur par module) ;
    - boutons et toggles stylés dans le même esprit.
*/
class WavefrontLookAndFeel : public juce::LookAndFeel_V4
{
public:
    WavefrontLookAndFeel();
    ~WavefrontLookAndFeel() override = default;

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider&) override;

    void drawToggleButton (juce::Graphics&, juce::ToggleButton&,
                           bool shouldDrawButtonAsHighlighted,
                           bool shouldDrawButtonAsDown) override;

    void drawButtonBackground (juce::Graphics&, juce::Button&,
                               const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override;

    void drawButtonText (juce::Graphics&, juce::TextButton&,
                         bool shouldDrawButtonAsHighlighted,
                         bool shouldDrawButtonAsDown) override;

    juce::Font getLabelFont (juce::Label&) override;
    void drawLabel (juce::Graphics&, juce::Label&) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WavefrontLookAndFeel)
};

} // namespace wavefront::ui
