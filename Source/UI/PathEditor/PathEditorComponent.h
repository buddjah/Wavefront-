#pragma once

#include <JuceHeader.h>

namespace wavefront { class WavefrontAudioProcessor; }

namespace wavefront::ui
{

/**
    Éditeur de trajectoire 2D.

    - dessine le champ (grille + anneaux de distance), la trajectoire (spline),
      les points de contrôle éditables, l'auditeur et le playhead animé ;
    - le playhead lit en lock-free la phase/position publiées par le DopplerEngine
      (Timer 60 Hz -> repaint) ;
    - trois modes :
        * Path     : glisser les points de contrôle pour éditer la spline ;
        * Listener : glisser l'auditeur (paramètres listenerX/Y) ;
        * Measure  : cliquer deux points pour mesurer une distance (en mètres).
*/
class PathEditorComponent : public juce::Component,
                            private juce::Timer
{
public:
    enum class Mode { Path, Listener, Measure };

    explicit PathEditorComponent (WavefrontAudioProcessor& proc);
    ~PathEditorComponent() override;

    void setMode (Mode m) { mode = m; measureA = measureB = {}; hasMeasure = false; repaint(); }
    Mode getMode() const noexcept { return mode; }

    void paint (juce::Graphics&) override;
    void resized() override;

    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp   (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    void timerCallback() override;

    juce::Rectangle<float> fieldBounds() const;
    juce::Point<float> normToPix (juce::Point<float> n) const;
    juce::Point<float> pixToNorm (juce::Point<float> p) const;

    int hitTestControlPoint (juce::Point<float> pix) const;
    juce::Point<float> listenerNorm() const;
    void setListenerFromPix (juce::Point<float> pix);
    float worldScale() const;

    WavefrontAudioProcessor& processor;

    Mode mode = Mode::Path;
    int  draggedPoint = -1;

    // Mesure.
    juce::Point<float> measureA, measureB;
    bool hasMeasure = false;
    bool draggingListener = false;

    // Couleurs (préfiguration du thème néon — module Path = violet).
    const juce::Colour colField   { 0xff0a0a12 };
    const juce::Colour colGrid     { 0x1affffff };
    const juce::Colour colPath     { 0xff9d4edd };
    const juce::Colour colPathGlow { 0x559d4edd };
    const juce::Colour colPoint    { 0xffc77dff };
    const juce::Colour colListener { 0xff48cae4 };
    const juce::Colour colPlayhead { 0xffff9e00 };
    const juce::Colour colMeasure  { 0xff90e0ef };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PathEditorComponent)
};

} // namespace wavefront::ui
