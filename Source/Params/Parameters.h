#pragma once

#include <JuceHeader.h>

namespace wavefront::params
{

/** Construit la disposition complète des paramètres de Wavefront (tous modules). */
juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

} // namespace wavefront::params
