#include "PathEditorComponent.h"
#include "../../PluginProcessor.h"
#include "../../Params/ParameterIDs.h"

namespace wavefront::ui
{

PathEditorComponent::PathEditorComponent (WavefrontAudioProcessor& proc)
    : processor (proc)
{
    setOpaque (true);
    startTimerHz (60);
}

PathEditorComponent::~PathEditorComponent()
{
    stopTimer();
}

// ---- Géométrie ------------------------------------------------------------

juce::Rectangle<float> PathEditorComponent::fieldBounds() const
{
    auto b = getLocalBounds().toFloat().reduced (18.0f);
    const float side = juce::jmin (b.getWidth(), b.getHeight());
    return juce::Rectangle<float> (side, side).withCentre (b.getCentre());
}

juce::Point<float> PathEditorComponent::normToPix (juce::Point<float> n) const
{
    auto f = fieldBounds();
    const float r = f.getWidth() * 0.5f;
    const auto c = f.getCentre();
    return { c.x + n.x * r, c.y - n.y * r }; // y écran vers le bas
}

juce::Point<float> PathEditorComponent::pixToNorm (juce::Point<float> p) const
{
    auto f = fieldBounds();
    const float r = f.getWidth() * 0.5f;
    const auto c = f.getCentre();
    return { (p.x - c.x) / r, -(p.y - c.y) / r };
}

float PathEditorComponent::worldScale() const
{
    if (auto* v = processor.getValueTree().getRawParameterValue (params::doppler::worldScale))
        return v->load();
    return 20.0f;
}

juce::Point<float> PathEditorComponent::listenerNorm() const
{
    auto& t = processor.getValueTree();
    const float x = t.getRawParameterValue (params::doppler::listenerX)->load();
    const float y = t.getRawParameterValue (params::doppler::listenerY)->load();
    return { x, y };
}

// ---- Rendu ----------------------------------------------------------------

void PathEditorComponent::paint (juce::Graphics& g)
{
    g.fillAll (colField);
    auto f = fieldBounds();

    // Cadre.
    g.setColour (colGrid);
    g.drawRoundedRectangle (f, 6.0f, 1.0f);

    // Anneaux de distance + axes.
    const auto centre = f.getCentre();
    const float r = f.getWidth() * 0.5f;
    for (int k = 1; k <= 4; ++k)
    {
        const float rr = r * (float) k / 4.0f;
        g.setColour (colGrid.withMultipliedAlpha (0.6f));
        g.drawEllipse (juce::Rectangle<float> (rr * 2, rr * 2).withCentre (centre), 1.0f);
    }
    g.setColour (colGrid);
    g.drawLine (f.getX(), centre.y, f.getRight(), centre.y, 1.0f);
    g.drawLine (centre.x, f.getY(), centre.x, f.getBottom(), 1.0f);

    // Trajectoire : échantillonnage de la spline via PathModel::evaluate.
    auto& path = processor.getPathModel();
    juce::Path trajectory;
    const int steps = 256;
    for (int i = 0; i <= steps; ++i)
    {
        const float ph = (float) i / (float) steps;
        const auto pix = normToPix (path.evaluate (ph));
        if (i == 0) trajectory.startNewSubPath (pix);
        else        trajectory.lineTo (pix);
    }
    trajectory.closeSubPath();

    // Glow (double tracé) puis trait net.
    g.setColour (colPathGlow);
    g.strokePath (trajectory, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved));
    g.setColour (colPath);
    g.strokePath (trajectory, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved));

    // Points de contrôle (éditables en mode Path).
    if (mode == Mode::Path)
    {
        const auto& cps = path.getControlPoints();
        for (int i = 0; i < (int) cps.size(); ++i)
        {
            const auto pix = normToPix (cps[(size_t) i]);
            const float rad = (i == draggedPoint) ? 8.0f : 5.5f;
            g.setColour (colPathGlow);
            g.fillEllipse (juce::Rectangle<float> (rad * 2.4f, rad * 2.4f).withCentre (pix));
            g.setColour (colPoint);
            g.fillEllipse (juce::Rectangle<float> (rad * 2, rad * 2).withCentre (pix));
        }
    }

    // Auditeur.
    {
        const auto pix = normToPix (listenerNorm());
        g.setColour (colListener.withAlpha (0.35f));
        g.fillEllipse (juce::Rectangle<float> (22, 22).withCentre (pix));
        g.setColour (colListener);
        g.drawEllipse (juce::Rectangle<float> (14, 14).withCentre (pix), 2.0f);
        g.fillEllipse (juce::Rectangle<float> (4, 4).withCentre (pix));
    }

    // Playhead animé (position courante de la source).
    {
        const float sx = processor.getDopplerEngine().getSourceX();
        const float sy = processor.getDopplerEngine().getSourceY();
        const auto pix = normToPix ({ sx, sy });
        const float pulse = 0.5f + 0.5f * std::sin ((float) juce::Time::getMillisecondCounter() * 0.006f);
        g.setColour (colPlayhead.withAlpha (0.25f + 0.25f * pulse));
        g.fillEllipse (juce::Rectangle<float> (26, 26).withCentre (pix));
        g.setColour (colPlayhead);
        g.fillEllipse (juce::Rectangle<float> (9, 9).withCentre (pix));

        // Ligne source -> auditeur (rayon de propagation courant).
        g.setColour (colPlayhead.withAlpha (0.5f));
        g.drawLine ({ pix, normToPix (listenerNorm()) }, 1.0f);
    }

    // Mesure.
    if (mode == Mode::Measure && hasMeasure)
    {
        const auto a = normToPix (measureA);
        const auto b = normToPix (measureB);
        g.setColour (colMeasure);
        g.drawLine ({ a, b }, 1.5f);
        g.fillEllipse (juce::Rectangle<float> (6, 6).withCentre (a));
        g.fillEllipse (juce::Rectangle<float> (6, 6).withCentre (b));

        const float distM = measureA.getDistanceFrom (measureB) * worldScale();
        g.setFont (juce::Font (juce::FontOptions (13.0f)));
        g.drawText (juce::String (distM, 1) + " m",
                    juce::Rectangle<float> (a.getX(), a.getY() - 20.0f, 120.0f, 18.0f),
                    juce::Justification::left, false);
    }

    // Étiquette du mode.
    g.setColour (juce::Colours::white.withAlpha (0.4f));
    g.setFont (juce::Font (juce::FontOptions (12.0f)));
    const char* modeName = mode == Mode::Path ? "PATH" : (mode == Mode::Listener ? "LISTENER" : "MEASURE");
    g.drawText (modeName, getLocalBounds().reduced (8), juce::Justification::topRight, false);
}

void PathEditorComponent::resized() {}

// ---- Interaction ----------------------------------------------------------

int PathEditorComponent::hitTestControlPoint (juce::Point<float> pix) const
{
    const auto& cps = processor.getPathModel().getControlPoints();
    int best = -1;
    float bestDist = 14.0f; // rayon de capture en pixels
    for (int i = 0; i < (int) cps.size(); ++i)
    {
        const float d = normToPix (cps[(size_t) i]).getDistanceFrom (pix);
        if (d < bestDist) { bestDist = d; best = i; }
    }
    return best;
}

void PathEditorComponent::setListenerFromPix (juce::Point<float> pix)
{
    auto n = pixToNorm (pix);
    n.x = juce::jlimit (-1.0f, 1.0f, n.x);
    n.y = juce::jlimit (-1.0f, 1.0f, n.y);
    auto& t = processor.getValueTree();
    if (auto* px = t.getParameter (params::doppler::listenerX))
        px->setValueNotifyingHost (px->convertTo0to1 (n.x));
    if (auto* py = t.getParameter (params::doppler::listenerY))
        py->setValueNotifyingHost (py->convertTo0to1 (n.y));
}

void PathEditorComponent::mouseDown (const juce::MouseEvent& e)
{
    const auto pix = e.position;

    switch (mode)
    {
        case Mode::Path:
            draggedPoint = hitTestControlPoint (pix);
            break;

        case Mode::Listener:
            draggingListener = true;
            setListenerFromPix (pix);
            break;

        case Mode::Measure:
            if (! hasMeasure) { measureA = pixToNorm (pix); measureB = measureA; hasMeasure = true; }
            else              { measureB = pixToNorm (pix); }
            break;
    }
    repaint();
}

void PathEditorComponent::mouseDrag (const juce::MouseEvent& e)
{
    const auto pix = e.position;

    if (mode == Mode::Path && draggedPoint >= 0)
    {
        processor.getPathModel().moveControlPoint (draggedPoint, pixToNorm (pix));
    }
    else if (mode == Mode::Listener && draggingListener)
    {
        setListenerFromPix (pix);
    }
    else if (mode == Mode::Measure && hasMeasure)
    {
        measureB = pixToNorm (pix);
    }
    repaint();
}

void PathEditorComponent::mouseUp (const juce::MouseEvent&)
{
    draggedPoint = -1;
    draggingListener = false;
}

void PathEditorComponent::mouseDoubleClick (const juce::MouseEvent& e)
{
    // Double-clic en mode Path : réinitialise la trajectoire par défaut.
    if (mode == Mode::Path)
    {
        auto& path = processor.getPathModel();
        path.setDefaultShape();
        path.rebuild();
        repaint();
    }
    juce::ignoreUnused (e);
}

void PathEditorComponent::timerCallback()
{
    repaint();
}

} // namespace wavefront::ui
