#include "PluginEditor.h"

using namespace juce;

namespace
{
constexpr int W = 300, H = 580;
String U (const char* s) { return sprak::tr (String::fromUTF8 (s)); }
}

//==============================================================================
DosLnF::DosLnF()
{
    setColour (ResizableWindow::backgroundColourId, stil::ground);
    setColour (PopupMenu::backgroundColourId, stil::raised);
    setColour (PopupMenu::textColourId, stil::ink);
    setColour (PopupMenu::highlightedBackgroundColourId, stil::sodium.withAlpha (0.25f));
    setColour (PopupMenu::highlightedTextColourId, stil::ink);
    setColour (PopupMenu::headerTextColourId, stil::ink3);
    setColour (TooltipWindow::backgroundColourId, stil::raised);
    setColour (TooltipWindow::textColourId, stil::ink);
}

// Den stora ratten: streckad skala, orange båge upp till värdet, kolgrått lock, vit visare, värdet i mitten.
void DosLnF::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, Slider& s)
{
    const auto area = Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    if (s.getProperties().contains ("mini")) { mini::draw (g, area, pos, a0, a1, s); return; }
    const float size = jmin (area.getWidth(), area.getHeight());
    const auto c = area.getCentre();
    const float ringR = size * 0.5f - 8.0f;
    const float angle = a0 + pos * (a1 - a0);
    const float thick = jmax (6.0f, size * 0.042f);

    Path track;
    track.addCentredArc (c.x, c.y, ringR, ringR, 0.0f, a0, a1, true);
    Path dashed;
    const float dashes[] = { 2.0f, 7.0f };
    PathStrokeType (thick).createDashedStroke (dashed, track, dashes, 2);
    g.setColour (stil::line2);
    g.fillPath (dashed);

    if (pos > 0.002f)
    {
        Path value;
        value.addCentredArc (c.x, c.y, ringR, ringR, 0.0f, a0, angle, true);
        g.setColour (stil::sodium);
        g.strokePath (value, PathStrokeType (thick));
    }

    const float capR = size * 0.325f;
    g.setColour (stil::raised);
    g.fillEllipse (Rectangle<float> (capR * 2, capR * 2).withCentre (c));
    g.setColour (s.isMouseOverOrDragging() ? stil::ink3 : stil::rim);
    g.drawEllipse (Rectangle<float> (capR * 2, capR * 2).withCentre (c).reduced (0.75f), 1.5f);
    const float inR = size * 0.25f;
    g.setColour (stil::panel);
    g.fillEllipse (Rectangle<float> (inR * 2, inR * 2).withCentre (c));

    const auto p0 = c.getPointOnCircumference (size * 0.19f, angle);
    const auto p1 = c.getPointOnCircumference (capR * 0.95f, angle);
    g.setColour (stil::ink);
    g.drawLine ({ p0, p1 }, jmax (3.0f, size * 0.021f));

    g.setColour (stil::sodium);
    g.setFont (stil::mono (size * 0.125f));
    g.drawText (String (roundToInt (s.getValue() * 100.0)), Rectangle<float> (size * 0.3f, size * 0.16f).withCentre (c), Justification::centred, false);
}

// De små reglagen: en turkos stapel och en vit markör.
void DosLnF::drawLinearSlider (Graphics& g, int x, int y, int w, int h, float pos, float, float, Slider::SliderStyle, Slider&)
{
    const auto r = Rectangle<float> ((float) x, (float) y + (float) h * 0.5f - 3.0f, (float) w, 6.0f);
    g.setColour (stil::cell);
    g.fillRect (r);
    g.setColour (stil::cold);
    g.fillRect (r.withRight (jlimit (r.getX(), r.getRight(), pos)));
    g.setColour (stil::ink);
    g.fillRect (Rectangle<float> (jlimit (r.getX(), r.getRight() - 2.0f, pos - 1.0f), r.getCentreY() - 7.0f, 2.0f, 14.0f));
}

void DosLnF::drawButtonBackground (Graphics& g, Button& b, const Colour&, bool over, bool down)
{
    const auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const bool bare = (bool) b.getProperties().getWithDefault ("bare", false);
    if (bare)
    {
        if (over || down) { g.setColour (stil::raised); g.fillRect (r); }
        return;
    }
    const bool on = b.getToggleState();
    g.setColour (down ? stil::raised.brighter (0.06f) : (over ? stil::raised : stil::panel));
    g.fillRect (r);
    if (on) { g.setColour (stil::sodium.withAlpha (0.16f)); g.fillRect (r); }
    g.setColour (on ? stil::sodium : (over ? stil::ink3 : stil::line2));
    g.drawRect (r, 1.0f);
}

void DosLnF::drawButtonText (Graphics& g, TextButton& b, bool, bool)
{
    const bool monoText = (bool) b.getProperties().getWithDefault ("mono", false);
    g.setFont (monoText ? stil::mono (12.0f) : stil::text (jmin (13.0f, (float) b.getHeight() * 0.42f), true, 0.12f));
    g.setColour (b.getToggleState() || monoText ? stil::ink : stil::ink2);
    g.drawFittedText (b.getButtonText(), b.getLocalBounds().reduced (4, 0), Justification::centred, 1, 0.8f);
}

void DosLnF::drawPopupMenuBackground (Graphics& g, int w, int h)
{
    g.fillAll (stil::raised);
    g.setColour (stil::line2);
    g.drawRect (0, 0, w, h, 1);
}

void DosLnF::drawTooltip (Graphics& g, const String& t, int w, int h)
{
    g.fillAll (stil::raised);
    g.setColour (stil::line2);
    g.drawRect (0, 0, w, h, 1);
    g.setColour (stil::ink);
    g.setFont (stil::text (12.5f, false, 0.0f));
    g.drawFittedText (t, Rectangle<int> (w, h).reduced (8, 5), Justification::centredLeft, 6);
}

//==============================================================================
DoseringPanel::DoseringPanel (DoseringProcessor& p, std::function<void()> bytSprak) : proc (p)
{
    knob.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    knob.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
    knob.setRotaryParameters (MathConstants<float>::pi * 1.25f, MathConstants<float>::pi * 2.75f, true);
    knob.setTooltip (U ("Hur stor dos botten får. Först duckar ljudet i takt, sedan fylls luckan med sub, sist rivs botten. Dubbelklicka för 50."));
    addAndMakeVisible (knob);
    knobAtt = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, "mangd", knob);
    knob.setDoubleClickReturnValue (true, 0.5);

    for (auto* s : { &mix, &ut })
    {
        s->setSliderStyle (Slider::LinearHorizontal);
        s->setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
        addAndMakeVisible (*s);
    }
    mix.setTooltip (U ("Blanda in det orörda ljudet. Lågt värde ger parallellt tryck."));
    ut.setTooltip (U ("Nivå ut."));
    mixAtt = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, "mix", mix);
    utAtt  = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, "ut", ut);
    mix.setDoubleClickReturnValue (true, 1.0);
    ut.setDoubleClickReturnValue (true, 0.0);

    // karaktären: de små rattarna. Övre raden formar pumpen, nedre raden suben och rivet.
    auto procent = [] (double v) { return String (roundToInt (v * 100.0)); };
    mini::setup (minis[0], *this, proc.apvts, "duck",  U ("DUCK"),  U ("Hur djupt ljudet duckar när den stora ratten är uppe."), procent);
    mini::setup (minis[1], *this, proc.apvts, "langd", U ("LÄNGD"), U ("Hur stor del av takten pumpen tar innan ljudet är tillbaka."), procent);
    mini::setup (minis[2], *this, proc.apvts, "form",  U ("FORM"),  U ("Pumpens böj. Lågt kommer tillbaka direkt, högt ligger kvar nere och kommer snabbt på slutet."),
                 [] (double v) { return String (v, 1); });
    mini::setup (minis[3], *this, proc.apvts, "anslag", U ("ANSLAG"), U ("Vänster mjukar upp slaget i ljudet, höger gör det hårdare."),
                 [] (double v) { return (v > 0.005 ? "+" : "") + String (roundToInt (v * 100.0)); }, true);
    mini::setup (minis[4], *this, proc.apvts, "sub",   U ("SUB"),   U ("Hur mycket sub som fylls på i luckan efter kicken."), procent);
    mini::setup (minis[5], *this, proc.apvts, "subhz", U ("SUBTON"), U ("Subens tonhöjd. Stäm den mot låtens grundton."),
                 [] (double v) { return String (v, 1) + " Hz"; });
    mini::setup (minis[6], *this, proc.apvts, "subfall", U ("FALL"), U ("Hur länge suben klingar innan den dör ut."),
                 [] (double v) { return String (roundToInt (v * 1000.0)) + " ms"; });
    mini::setup (minis[7], *this, proc.apvts, "dist",  U ("RIV"),   U ("Hur mycket botten rivs upp av mättning högst upp på den stora ratten."), procent);

    const char* namnen[4] = { "1/4", "1/8", "OFFBEAT", "LYSSNA" };
    const char* tipsen[4] = { "Pumpar på varje fjärdedel, i takt med DAW:en.", "Pumpar på varje åttondel.",
                              "Pumpar mitt emellan slagen.", "Lyssnar efter kicken i ljudet och pumpar på den. Behöver ingen takt." };
    for (int i = 0; i < 4; ++i)
    {
        lage[i].setButtonText (U (namnen[i]));
        lage[i].setTooltip (U (tipsen[i]));
        lage[i].onClick = [this, i] { setChoice (i); };
        addAndMakeVisible (lage[i]);
    }

    allt.setButtonText (U ("PUMPA ALLT"));
    allt.setClickingTogglesState (true);
    allt.setTooltip (U ("På: hela ljudet duckar. Av: bara botten duckar och toppen står kvar."));
    addAndMakeVisible (allt);
    alltAtt = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "allt", allt);

    slumpa.setButtonText (U ("SLUMPA"));
    slumpa.setTooltip (U ("Ny karaktär i samma läge. Ratten står kvar."));
    slumpa.onClick = [this] { proc.slumpa(); };
    addAndMakeVisible (slumpa);

    prev.setButtonText ("<");
    next.setButtonText (">");
    prev.setTooltip (U ("Föregående preset"));
    next.setTooltip (U ("Nästa preset"));
    namn.setTooltip (U ("Välj preset"));
    for (auto* b : { &prev, &next, &namn }) { b->getProperties().set ("bare", true); addAndMakeVisible (*b); }
    namn.getProperties().set ("mono", true);
    prev.onClick = [this] { proc.setCurrentProgram ((proc.getCurrentProgram() + dos::kNumPresets - 1) % dos::kNumPresets); };
    next.onClick = [this] { proc.setCurrentProgram ((proc.getCurrentProgram() + 1) % dos::kNumPresets); };
    namn.onClick = [this] { visaPresetMeny(); };

    sprakKnapp.accent = stil::sodium;
    sprakKnapp.dim = stil::ink3;
    sprakKnapp.kant = stil::line2;
    sprakKnapp.onClick = std::move (bytSprak);
    addAndMakeVisible (sprakKnapp);

    setSize (W, H);
    timerCallback();
    startTimerHz (30);
}

DoseringPanel::~DoseringPanel() { stopTimer(); }

void DoseringPanel::setChoice (int mode)
{
    if (auto* p = proc.apvts.getParameter ("lage"))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 ((float) mode));
        p->endChangeGesture();
    }
}

void DoseringPanel::visaPresetMeny()
{
    PopupMenu m;
    const char* grupper[4] = { "1/4", "1/8", "OFFBEAT", "LYSSNA" };
    const int cur = proc.getCurrentProgram();
    for (int g = 0; g < 4; ++g)
    {
        PopupMenu sub;
        for (int i = 0; i < dos::kNumPresets; ++i)
            if (dos::kPresets[i].mode == g)
                sub.addItem (i + 1, String (i + 1).paddedLeft ('0', 2) + "  " + String::fromUTF8 (dos::kPresets[i].name), true, i == cur);
        m.addSubMenu (U (grupper[g]), sub);
    }
    Component::SafePointer<DoseringPanel> self (this);
    m.showMenuAsync (PopupMenu::Options().withTargetComponent (&namn), [self] (int id)
    {
        if (self != nullptr && id > 0) self->proc.setCurrentProgram (id - 1);
    });
}

void DoseringPanel::timerCallback()
{
    // lamporna: stiger direkt, faller mjukt
    lDuck = jmax (proc.mDuck.load(), lDuck * 0.82f);
    lSub  = jmax (proc.mSub.load(),  lSub * 0.82f);
    lDist = jmax (proc.mDist.load(), lDist * 0.82f);

    const int mode = jlimit (0, 3, roundToInt (proc.apvts.getRawParameterValue ("lage")->load()));
    for (int i = 0; i < 4; ++i) lage[i].setToggleState (i == mode, dontSendNotification);

    const int pr = proc.getCurrentProgram();
    if (pr != shownPreset)
    {
        shownPreset = pr;
        namn.setButtonText (String (pr + 1).paddedLeft ('0', 2) + " " + proc.getProgramName (pr));
    }
    repaint();
}

void DoseringPanel::resized()
{
    auto r = getLocalBounds();
    auto head = r.removeFromTop (58);
    sprakKnapp.setBounds (head.getRight() - 14 - 46, head.getCentreY() - 10, 46, 20);

    auto foot = r.removeFromBottom (44);
    prev.setBounds (foot.removeFromLeft (44));
    next.setBounds (foot.removeFromRight (44));
    namn.setBounds (foot);

    knobArea = r.removeFromTop (178);
    knob.setBounds (knobArea.withSizeKeepingCentre (172, 172));
    lampArea = r.removeFromTop (26);

    miniArea = r.removeFromTop (116).reduced (10, 0);
    mini::layoutRow (minis, 4, miniArea.withHeight (58));
    mini::layoutRow (minis + 4, 4, miniArea.withTrimmedTop (58));
    r.removeFromTop (6);

    auto modes = r.removeFromTop (40).reduced (14, 0);
    const int bw = (modes.getWidth() - 15) / 4;
    for (int i = 0; i < 4; ++i) { lage[i].setBounds (modes.removeFromLeft (bw)); modes.removeFromLeft (5); }

    r.removeFromTop (10);
    mixRow = r.removeFromTop (24).reduced (14, 0);
    utRow  = r.removeFromTop (24).reduced (14, 0);
    mix.setBounds (mixRow.withTrimmedLeft (40).withTrimmedRight (52));
    ut.setBounds (utRow.withTrimmedLeft (40).withTrimmedRight (52));

    r.removeFromTop (8);
    auto row = r.removeFromTop (34).reduced (14, 0);
    allt.setBounds (row.removeFromLeft ((row.getWidth() - 6) / 2));
    row.removeFromLeft (6);
    slumpa.setBounds (row);
}

void DoseringPanel::paint (Graphics& g)
{
    g.fillAll (stil::ground);

    // huvudet
    g.setColour (stil::ink);
    g.setFont (stil::text (27.0f, true, 0.2f));
    g.drawText ("DOSERING", 14, 10, 200, 28, Justification::centredLeft, false);
    g.setColour (stil::ink3);
    g.setFont (stil::text (10.5f, false, 0.18f));
    g.drawText (String::fromUTF8 ("MASKINHALLEN \xc2\xb7 ONE KNOB"), 14, 38, 200, 12, Justification::centredLeft, false);
    g.setColour (stil::line);
    g.fillRect (0, 57, W, 1);
    g.fillRect (0, H - 45, W, 1);

    // lamporna: vad som händer just nu
    struct Lampa { const char* namn; float styrka; };
    const Lampa lampor[3] = { { "DUCKAR", jlimit (0.0f, 1.0f, lDuck * 1.5f) }, { "FYLLER", jlimit (0.0f, 1.0f, lSub * 6.0f) }, { "RIVER", jlimit (0.0f, 1.0f, lDist * 2.0f) } };
    const int lw = (W - 28) / 3;
    for (int i = 0; i < 3; ++i)
    {
        const auto cellR = Rectangle<int> (14 + i * lw, lampArea.getY(), lw, lampArea.getHeight());
        const bool on = lampor[i].styrka > 0.12f;
        const auto text = U (lampor[i].namn);
        g.setFont (stil::text (12.0f, true, 0.12f));
        const int tw = (int) GlyphArrangement::getStringWidth (g.getCurrentFont(), text);
        const int x0 = cellR.getCentreX() - (tw + 14) / 2;
        const auto dot = Rectangle<float> ((float) x0, (float) cellR.getCentreY() - 4.0f, 8.0f, 8.0f);
        if (on) { g.setColour (stil::sodium.withAlpha (0.35f + 0.65f * lampor[i].styrka)); g.fillRect (dot); }
        else    { g.setColour (stil::ink3); g.drawRect (dot, 1.0f); }
        g.setColour (on ? stil::ink : stil::ink3);
        g.drawText (text, x0 + 14, cellR.getY(), tw + 4, cellR.getHeight(), Justification::centredLeft, false);
    }

    mini::paintLabels (g, minis, 8);

    // de små reglagen: namn till vänster, värde till höger
    auto rad = [&g] (Rectangle<int> r, const String& namnText, const String& varde)
    {
        g.setColour (stil::ink3);
        g.setFont (stil::text (11.5f, true, 0.12f));
        g.drawText (namnText, r.withWidth (38), Justification::centredLeft, false);
        g.setColour (stil::ink2);
        g.setFont (stil::mono (11.0f));
        g.drawText (varde, r.withTrimmedLeft (r.getWidth() - 50), Justification::centredRight, false);
    };
    rad (mixRow, "MIX", String (roundToInt (mix.getValue() * 100.0)) + " %");
    const double db = ut.getValue();
    rad (utRow, U ("UT"), (db > 0.05 ? "+" : "") + String (db, 1) + " dB");
}

// Turkos båge innanför rattens ring: hur djupt ljudet är nerduckat just nu.
void DoseringPanel::paintOverChildren (Graphics& g)
{
    const float gr = jlimit (0.0f, 1.0f, lDuck);
    if (gr < 0.02f) return;
    const auto b = knob.getBounds().toFloat();
    const auto c = b.getCentre();
    const float r = jmin (b.getWidth(), b.getHeight()) * 0.5f - 18.0f;
    const float a0 = MathConstants<float>::pi * 1.25f, a1 = MathConstants<float>::pi * 2.75f;
    Path arc;
    arc.addCentredArc (c.x, c.y, r, r, 0.0f, a0, a0 + gr * (a1 - a0), true);
    g.setColour (stil::cold);
    g.strokePath (arc, PathStrokeType (3.0f));
}

//==============================================================================
DoseringEditor::DoseringEditor (DoseringProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);
    tips.setLookAndFeel (&lnf);
    sprak::laddaOm();
    byggPanel();
    setResizable (true, true);
    setResizeLimits (W, H, W * 2, H * 2);
    if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) W / (double) H);
    setSize (W, H);
}

DoseringEditor::~DoseringEditor()
{
    panel.reset();
    tips.setLookAndFeel (nullptr);
    setLookAndFeel (nullptr);
}

void DoseringEditor::byggPanel()
{
    panel.reset();
    Component::SafePointer<DoseringEditor> self (this);
    panel = std::make_unique<DoseringPanel> (proc, [self]
    {
        sprak::sattEngelska (! sprak::engelska());
        MessageManager::callAsync ([self] { if (self != nullptr) self->byggPanel(); });
    });
    addAndMakeVisible (*panel);
    resized();
}

void DoseringEditor::resized()
{
    if (panel == nullptr) return;
    // panelen ritas alltid som 300 x 540 och skalas upp när fönstret dras större
    panel->setTransform (AffineTransform::scale ((float) getWidth() / (float) W));
    panel->setBounds (0, 0, W, H);
}
