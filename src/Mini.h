#pragma once

// MASKINHALLEN · ONE KNOB: de små rattarna under den stora. Tas in efter namnrymden stil i PluginEditor.h.
// Varje liten ratt har ett namn och ett värde i klartext under sig, så man ser vad den står på utan att röra den.

namespace mini
{
struct Knob
{
    juce::Slider slider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
    juce::String label;
    std::function<juce::String (double)> text;
    juce::Rectangle<int> cell;
};

inline void setup (Knob& k, juce::Component& parent, juce::AudioProcessorValueTreeState& apvts, const char* id,
                   const juce::String& label, const juce::String& tip, std::function<juce::String (double)> text, bool bipolar = false)
{
    k.label = label;
    k.text = std::move (text);
    k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    k.slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    k.slider.getProperties().set ("mini", true);
    k.slider.getProperties().set ("bipolar", bipolar);
    k.slider.setTooltip (tip);
    parent.addAndMakeVisible (k.slider);
    k.att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, id, k.slider);
    if (auto* p = apvts.getParameter (id))
        k.slider.setDoubleClickReturnValue (true, (double) p->convertFrom0to1 (p->getDefaultValue()));
}

// Ritar en liten ratt: tunn skala, orange båge upp till värdet (från mitten om ratten går åt två håll), vit visare.
inline void draw (juce::Graphics& g, juce::Rectangle<float> area, float pos, float a0, float a1, juce::Slider& s)
{
    const float size = juce::jmin (area.getWidth(), area.getHeight());
    const auto c = area.getCentre();
    const float r = size * 0.5f - 2.5f;
    const float angle = a0 + pos * (a1 - a0);
    const bool bipolar = (bool) s.getProperties().getWithDefault ("bipolar", false);

    juce::Path track;
    track.addCentredArc (c.x, c.y, r, r, 0.0f, a0, a1, true);
    g.setColour (stil::line2);
    g.strokePath (track, juce::PathStrokeType (3.0f));

    const float from = bipolar ? (a0 + a1) * 0.5f : a0;
    if (std::abs (angle - from) > 0.02f)
    {
        juce::Path value;
        value.addCentredArc (c.x, c.y, r, r, 0.0f, juce::jmin (from, angle), juce::jmax (from, angle), true);
        g.setColour (s.isEnabled() ? stil::sodium : stil::ink3);
        g.strokePath (value, juce::PathStrokeType (3.0f));
    }

    const float capR = r - 4.5f;
    g.setColour (stil::raised);
    g.fillEllipse (juce::Rectangle<float> (capR * 2, capR * 2).withCentre (c));
    g.setColour (s.isMouseOverOrDragging() ? stil::ink3 : stil::rim);
    g.drawEllipse (juce::Rectangle<float> (capR * 2, capR * 2).withCentre (c), 1.0f);
    g.setColour (stil::ink);
    g.drawLine ({ c.getPointOnCircumference (capR * 0.3f, angle), c.getPointOnCircumference (capR * 0.92f, angle) }, 2.0f);
}

// Lägger n rattar jämnt i en rad. Ratten överst i varje ruta, namn och värde under.
inline void layoutRow (Knob* knobs, int n, juce::Rectangle<int> row)
{
    const int cw = row.getWidth() / n;
    for (int i = 0; i < n; ++i)
    {
        knobs[i].cell = juce::Rectangle<int> (row.getX() + i * cw, row.getY(), cw, row.getHeight());
        knobs[i].slider.setBounds (knobs[i].cell.withHeight (row.getHeight() - 24).withSizeKeepingCentre (row.getHeight() - 24, row.getHeight() - 24));
    }
}

inline void paintLabels (juce::Graphics& g, Knob* knobs, int n)
{
    for (int i = 0; i < n; ++i)
    {
        auto& k = knobs[i];
        const auto under = k.cell.withTrimmedTop (k.cell.getHeight() - 24);
        g.setColour (stil::ink2);
        g.setFont (stil::text (10.5f, true, 0.08f));
        g.drawFittedText (k.label, under.withHeight (12), juce::Justification::centred, 1, 0.75f);
        g.setColour (k.slider.isMouseOverOrDragging() ? stil::sodium : stil::ink3);
        g.setFont (stil::mono (10.0f));
        g.drawText (k.text ? k.text (k.slider.getValue()) : juce::String(), under.withTrimmedTop (12), juce::Justification::centred, false);
    }
}
} // namespace mini
