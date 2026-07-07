#pragma once

#include <JuceHeader.h>

namespace wavefront::ui::colours
{

// Fonds sombres.
inline const juce::Colour background { 0xff07070d };
inline const juce::Colour field      { 0xff0a0a12 };
inline const juce::Colour panel      { 0xff12121c };
inline const juce::Colour panelEdge  { 0x22ffffff };

// Texte.
inline const juce::Colour text     { 0xffe8e8f0 };
inline const juce::Colour dimText   { 0xff8a8a9a };

// Accents néon par module (voir cahier des charges).
inline const juce::Colour path      { 0xff9d4edd }; // Path      = violet
inline const juce::Colour physics   { 0xff48cae4 }; // Physique  = cyan
inline const juce::Colour sampler   { 0xff52ffb8 }; // Sampler   = vert
inline const juce::Colour effects   { 0xff2ec4b6 }; // Effets    = turquoise
inline const juce::Colour modMatrix { 0xffff9e00 }; // Mod Matrix= orange

} // namespace wavefront::ui::colours
