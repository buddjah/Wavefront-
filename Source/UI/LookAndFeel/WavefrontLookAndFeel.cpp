#include "WavefrontLookAndFeel.h"

namespace wavefront::ui
{

WavefrontLookAndFeel::WavefrontLookAndFeel()
{
    using namespace colours;

    setColour (juce::ResizableWindow::backgroundColourId, background);
    setColour (juce::PopupMenu::backgroundColourId, panel);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, path.withAlpha (0.4f));

    setColour (juce::Slider::rotarySliderFillColourId, physics);
    setColour (juce::Slider::textBoxTextColourId, text);
    setColour (juce::Slider::textBoxBackgroundColourId, panel);
    setColour (juce::Slider::textBoxOutlineColourId, panelEdge);

    setColour (juce::Label::textColourId, text);

    setColour (juce::TextButton::buttonColourId, panel);
    setColour (juce::TextButton::buttonOnColourId, path.withAlpha (0.25f));
    setColour (juce::TextButton::textColourOffId, dimText);
    setColour (juce::TextButton::textColourOnId, text);

    setColour (juce::ToggleButton::textColourId, text);
    setColour (juce::ToggleButton::tickColourId, sampler);
}

void WavefrontLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                             float sliderPos, float startAngle, float endAngle,
                                             juce::Slider& slider)
{
    using namespace colours;

    auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (4.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const float lineW = juce::jmax (2.5f, radius * 0.14f);
    const float arcRadius = radius - lineW * 0.6f;

    const juce::Colour accent = slider.findColour (juce::Slider::rotarySliderFillColourId);
    const float angle = startAngle + sliderPos * (endAngle - startAngle);

    // Corps du bouton (léger dégradé radial).
    g.setGradientFill (juce::ColourGradient (panel.brighter (0.08f), centre,
                                             background, bounds.getBottomRight(), true));
    g.fillEllipse (juce::Rectangle<float> (arcRadius * 1.35f, arcRadius * 1.35f).withCentre (centre));

    // Rail de fond.
    juce::Path back;
    back.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, startAngle, endAngle, true);
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.strokePath (back, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Arc de valeur — glow (large, translucide) puis trait net.
    if (sliderPos > 0.0f)
    {
        juce::Path value;
        value.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, startAngle, angle, true);

        g.setColour (accent.withAlpha (0.30f));
        g.strokePath (value, juce::PathStrokeType (lineW * 2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (accent);
        g.strokePath (value, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Pointeur néon.
    const juce::Point<float> tip (centre.x + arcRadius * std::cos (angle - juce::MathConstants<float>::halfPi),
                                  centre.y + arcRadius * std::sin (angle - juce::MathConstants<float>::halfPi));
    g.setColour (accent.withAlpha (0.35f));
    g.fillEllipse (juce::Rectangle<float> (lineW * 2.2f, lineW * 2.2f).withCentre (tip));
    g.setColour (accent.brighter (0.4f));
    g.fillEllipse (juce::Rectangle<float> (lineW * 1.1f, lineW * 1.1f).withCentre (tip));
}

void WavefrontLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                             bool highlighted, bool /*down*/)
{
    using namespace colours;
    auto bounds = button.getLocalBounds().toFloat();
    auto box = bounds.removeFromLeft (bounds.getHeight()).reduced (3.0f);
    const bool on = button.getToggleState();
    const juce::Colour accent = button.findColour (juce::ToggleButton::tickColourId);

    g.setColour (panel);
    g.fillRoundedRectangle (box, 4.0f);
    g.setColour (on ? accent : juce::Colours::white.withAlpha (highlighted ? 0.35f : 0.18f));
    g.drawRoundedRectangle (box, 4.0f, 1.5f);

    if (on)
    {
        g.setColour (accent.withAlpha (0.35f));
        g.fillRoundedRectangle (box.reduced (2.0f), 3.0f);
        g.setColour (accent);
        g.fillRoundedRectangle (box.reduced (box.getWidth() * 0.28f), 2.0f);
    }

    g.setColour (button.findColour (juce::ToggleButton::textColourId).withAlpha (on ? 1.0f : 0.7f));
    g.setFont (juce::Font (juce::FontOptions (13.0f)));
    g.drawText (button.getButtonText(), bounds.withTrimmedLeft (6.0f), juce::Justification::centredLeft, false);
}

void WavefrontLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                                 const juce::Colour& /*bg*/,
                                                 bool highlighted, bool down)
{
    using namespace colours;
    auto bounds = button.getLocalBounds().toFloat().reduced (1.0f);
    const bool on = button.getToggleState();

    // Accent : couleur "on" du bouton si définie, sinon violet Path.
    juce::Colour accent = button.findColour (juce::TextButton::buttonOnColourId).withMultipliedAlpha (4.0f);
    if (accent.isTransparent()) accent = path;

    g.setColour (panel.withMultipliedBrightness (down ? 0.8f : 1.0f));
    g.fillRoundedRectangle (bounds, 5.0f);

    if (on)
    {
        g.setColour (accent.withAlpha (0.18f));
        g.fillRoundedRectangle (bounds, 5.0f);
    }

    g.setColour (on ? accent : juce::Colours::white.withAlpha (highlighted ? 0.35f : 0.15f));
    g.drawRoundedRectangle (bounds, 5.0f, on ? 1.6f : 1.0f);
}

void WavefrontLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button,
                                           bool /*highlighted*/, bool /*down*/)
{
    g.setColour (button.findColour (button.getToggleState() ? juce::TextButton::textColourOnId
                                                            : juce::TextButton::textColourOffId));
    g.setFont (juce::Font (juce::FontOptions (13.0f)));
    g.drawText (button.getButtonText(), button.getLocalBounds(), juce::Justification::centred, false);
}

juce::Font WavefrontLookAndFeel::getLabelFont (juce::Label&)
{
    return juce::Font (juce::FontOptions (12.0f));
}

void WavefrontLookAndFeel::drawLabel (juce::Graphics& g, juce::Label& label)
{
    g.setColour (label.findColour (juce::Label::textColourId));
    g.setFont (getLabelFont (label));
    g.drawFittedText (label.getText(), label.getLocalBounds(),
                      label.getJustificationType(), 1, 0.9f);
}

} // namespace wavefront::ui
