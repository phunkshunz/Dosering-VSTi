#include "PluginEditor.h"

using namespace KoldColours;
static juce::String u8 (const char* s) { return sprak::trU (s); }
static constexpr int kW = 980, kH = 540;

//==============================================================================
Kurvan::Kurvan (KoldProcessor& p) : proc (p)
{
    setTooltip (u8 ("KURVAN: rita med musen hur botten ska ducka efter varje kick. Uppe = full nivå, nere = borta. Dubbelklicka för att börja om med PUMP."));
    setMouseCursor (juce::MouseCursor::CrosshairCursor);
    startTimerHz (30);
}

void Kurvan::paint (juce::Graphics& g)
{
    auto r = plot();
    g.setColour (ground);
    g.fillRect (r);
    // rutnät: fjärdedelar av kurvan och 25 %-linjer
    g.setColour (line);
    for (int i = 1; i < 8; ++i) g.drawVerticalLine (juce::roundToInt (r.getX() + r.getWidth() * i / 8.f), r.getY(), r.getBottom());
    for (int i = 1; i < 4; ++i) g.drawHorizontalLine (juce::roundToInt (r.getY() + r.getHeight() * i / 4.f), r.getX(), r.getRight());
    g.setColour (line2);
    g.drawRect (r, 1.f);

    // djupet: kurvan ritas som den verkligen låter (DJUP skalar hur långt ner den går)
    const float depth = proc.apvts.getRawParameterValue (ParamIDs::djup)->load();
    juce::Path fill, stroke;
    const int N = kold::kCurvePoints;
    for (int i = 0; i < N; ++i)
    {
        const float x = r.getX() + r.getWidth() * (float) i / (float) (N - 1);
        const float v = 1.f - depth * (1.f - proc.curve[(size_t) i].load());
        const float y = r.getBottom() - r.getHeight() * v;
        if (i == 0) { stroke.startNewSubPath (x, y); fill.startNewSubPath (x, r.getBottom()); }
        else stroke.lineTo (x, y);
        fill.lineTo (x, y);
    }
    fill.lineTo (r.getRight(), r.getBottom());
    fill.closeSubPath();
    g.setColour (cold.withAlpha (0.16f));
    g.fillPath (fill);
    // den ritade kurvan (utan djup), tunt
    juce::Path drawn;
    for (int i = 0; i < N; ++i)
    {
        const float x = r.getX() + r.getWidth() * (float) i / (float) (N - 1);
        const float y = r.getBottom() - r.getHeight() * proc.curve[(size_t) i].load();
        if (i == 0) drawn.startNewSubPath (x, y); else drawn.lineTo (x, y);
    }
    g.setColour (muted.withAlpha (0.6f));
    g.strokePath (drawn, juce::PathStrokeType (1.f));
    g.setColour (cold);
    g.strokePath (stroke, juce::PathStrokeType (2.f));
    for (int i = 0; i < N; ++i)
    {
        const float x = r.getX() + r.getWidth() * (float) i / (float) (N - 1);
        const float y = r.getBottom() - r.getHeight() * proc.curve[(size_t) i].load();
        g.fillEllipse (x - 2.f, y - 2.f, 4.f, 4.f);
    }

    // spelhuvudet
    const float pos = proc.curvePos.load();
    if (pos < 1.f)
    {
        const float x = r.getX() + r.getWidth() * pos;
        g.setColour (sodium);
        g.drawVerticalLine (juce::roundToInt (x), r.getY(), r.getBottom());
    }
    // nivåmätaren till höger
    const float gain = proc.meterDuck.load();
    auto m = juce::Rectangle<float> (r.getRight() + 3.f, r.getY(), 5.f, r.getHeight());
    g.setColour (raised);
    g.fillRect (m);
    g.setColour (sodium);
    g.fillRect (m.withTop (m.getBottom() - m.getHeight() * gain));

    g.setColour (muted);
    g.setFont (juce::FontOptions (9.5f));
    auto lab = getLocalBounds().toFloat().reduced (12.f, 0.f).removeFromBottom (14.f);
    g.drawText (u8 ("KICK"), lab, juce::Justification::centredLeft);
    g.drawText (u8 ("rita med musen"), lab, juce::Justification::centred);
    g.drawText (u8 ("SLUT"), lab, juce::Justification::centredRight);
}

void Kurvan::drawAt (juce::Point<float> pos)
{
    auto r = plot();
    const int N = kold::kCurvePoints;
    const int idx = juce::jlimit (0, N - 1, juce::roundToInt ((pos.x - r.getX()) / r.getWidth() * (float) (N - 1)));
    const float val = kold::clamp01 ((r.getBottom() - pos.y) / r.getHeight());
    if (lastIdx >= 0 && lastIdx != idx)
    {
        // fyll i punkterna mellan förra och den här, så att snabba drag inte lämnar hål
        const int step = idx > lastIdx ? 1 : -1;
        for (int i = lastIdx + step; i != idx; i += step)
            proc.setCurvePoint (i, lastVal + (val - lastVal) * (float) (i - lastIdx) / (float) (idx - lastIdx));
    }
    proc.setCurvePoint (idx, val);
    lastIdx = idx;
    lastVal = val;
    repaint();
}

void Kurvan::mouseDown (const juce::MouseEvent& e) { lastIdx = -1; drawAt (e.position); }
void Kurvan::mouseDrag (const juce::MouseEvent& e) { drawAt (e.position); }
void Kurvan::mouseDoubleClick (const juce::MouseEvent&)
{
    if (auto* prm = proc.apvts.getParameter (ParamIDs::form))
    {
        prm->setValueNotifyingHost (prm->convertTo0to1 (0.f));
        proc.applyForm (0);
    }
    repaint();
}

//==============================================================================
Knob& KoldPanel::knob (const char* id, const char* name, juce::Colour accent, const char* tip)
{
    auto k = std::make_unique<Knob>();
    k->attach (proc.apvts, id, juce::String::fromUTF8 (name));
    k->setAccent (accent);
    k->slider.setTooltip (u8 (tip));
    addAndMakeVisible (*k);
    knobs.push_back (std::move (k));
    return *knobs.back();
}

juce::ComboBox& KoldPanel::box (const char* id, const char* tip)
{
    auto b = std::make_unique<juce::ComboBox>();
    if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter (id)))
        for (int i = 0; i < p->choices.size(); ++i) b->addItem (sprak::tr (p->choices[i]), i + 1);
    boxAtts.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, id, *b));
    b->setTooltip (u8 (tip));
    addAndMakeVisible (*b);
    boxes.push_back (std::move (b));
    return *boxes.back();
}

KoldPanel::KoldPanel (KoldProcessor& p) : proc (p)
{
    addAndMakeVisible (kurvan);
    kalla   = &box (ParamIDs::kalla, "Vad som startar kurvan och giljotinen. TAKT: varje fjärdedel. SIDECHAIN: kicken på sidokedjan. SJÄLV: kickarna i signalen själv.");
    form    = &box (ParamIDs::form, "Färdiga former för kurvan. Ritar du blir det EGEN.");
    ducktid = &box (ParamIDs::ducktid, "Hur lång kurvan är, i takt med låten.");
    duckmal = &box (ParamIDs::duckmal, "BARA BOTTEN duckar under brytpunkten och låter toppen vara. ALLT duckar hela ljudet.");
    lage    = &box (ParamIDs::lage, "SLÄP: subbens svans får dö ut (techno). SPIKRAKT: svansen kapas kirurgiskt (psy).");

    djup       = &knob (ParamIDs::djup, "DJUP", cold, "Hur långt ner kurvan går.");
    kanslighet = &knob (ParamIDs::kanslighet, "KÄNSLIGHET", cold, "Hur lätt kicken hittas i SIDECHAIN och SJÄLV.");
    kicklangd  = &knob (ParamIDs::kicklangd, "KICKLÄNGD", cold, "I TAKT-läget: hur länge kicken anses låta. Giljotinen faller när den är slut.");
    subton     = &knob (ParamIDs::subton, "SUBTON", sodium, "Subbens ton. Lägg den på låtens grundton.");
    subniva    = &knob (ParamIDs::subniva, "SUBNIVÅ", sodium, "Hur stark subben är. Noll = giljotinen av.");
    subfall    = &knob (ParamIDs::subfall, "SUBFALL", sodium, "Hur länge subben ringer efter kicken.");
    anslag     = &knob (ParamIDs::anslag, "ANSLAG", concrete, "Skarpare eller mjukare anslag.");
    svans      = &knob (ParamIDs::svans, "SVANS", concrete, "Längre eller kortare svans.");
    brytpunkt  = &knob (ParamIDs::brytpunkt, "BRYTPUNKT", sodium, "Var botten slutar och toppen börjar.");
    subdist    = &knob (ParamIDs::subdist, "SUBDIST", sodium, "Mättar botten så att den hörs i små högtalare.");
    ton        = &knob (ParamIDs::ton, "TON", sodium, "Hur ljus mättningen får bli.");
    blandning  = &knob (ParamIDs::blandning, "BLANDNING", text, "Torrt mot behandlat.");
    niva       = &knob (ParamIDs::niva, "NIVÅ", text, "Utnivå.");

    monoBtn.setClickingTogglesState (true);
    monoBtn.setTooltip (u8 ("Botten under brytpunkten i mono. Säkrare i klubben."));
    monoAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, ParamIDs::monobas, monoBtn);
    addAndMakeVisible (monoBtn);
    startTimerHz (24);
}

void KoldPanel::timerCallback()
{
    subMeter = proc.meterSub.load();
    duckMeter = proc.meterDuck.load();
    const bool k = proc.meterKick.load();
    if (k != kickLamp) { kickLamp = k; }
    repaint (secGilj);
    repaint (getLocalBounds().removeFromTop (70));
}

void KoldPanel::paint (juce::Graphics& g)
{
    g.fillAll (ground);
    // huvud
    g.setColour (cold);
    g.setFont (juce::FontOptions (34.f, juce::Font::bold));
    g.drawText (juce::String::fromUTF8 ("KÖLDBÄRAREN"), 24, 14, 300, 40, juce::Justification::centredLeft);
    g.setColour (muted);
    g.setFont (juce::FontOptions (11.f));
    g.drawText (u8 ("MASKINHALLEN · BOTTEN-MASKINEN"), 26, 52, 300, 14, juce::Justification::centredLeft);

    // källans lampa
    const int src = juce::roundToInt (proc.apvts.getRawParameterValue (ParamIDs::kalla)->load());
    juce::String status;
    if (src == 1) status = proc.sidechainConnected.load() ? u8 ("SIDECHAIN INKOPPLAD") : u8 ("KOPPLA KICKEN TILL SIDOKEDJAN");
    else if (src == 0) status = u8 ("FÖLJER VÄRDENS TAKT");
    else status = u8 ("LYSSNAR PÅ SIG SJÄLV");
    g.setColour (src == 1 && ! proc.sidechainConnected.load() ? sodium : muted);
    g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
    g.drawText (status, kW - 470, 44, 300, 14, juce::Justification::centredLeft);
    g.setColour (kickLamp ? sodium : raised);
    g.fillEllipse ((float) kW - 190.f, 24.f, 12.f, 12.f);
    g.setColour (muted);
    g.setFont (juce::FontOptions (10.f));
    g.drawText (u8 ("KICK"), kW - 172, 22, 40, 16, juce::Justification::centredLeft);

    drawPanel (g, secKurva, u8 ("KURVAN"), u8 ("duckar botten efter varje kick"));
    drawPanel (g, secGilj, u8 ("GILJOTIN"), u8 ("faslåst sub efter kicken"));
    drawPanel (g, secTrans, u8 ("TRANSIENT"));
    drawPanel (g, secDist, u8 ("SUBDIST"));
    drawPanel (g, secUt, u8 ("UT"));

    // subbens mätare i GILJOTIN
    auto m = juce::Rectangle<float> ((float) secGilj.getRight() - 18.f, (float) secGilj.getY() + 34.f, 5.f, (float) secGilj.getHeight() - 48.f);
    g.setColour (raised);
    g.fillRect (m);
    g.setColour (sodium);
    g.fillRect (m.withTop (m.getBottom() - m.getHeight() * juce::jlimit (0.f, 1.f, subMeter * 1.4f)));
}

void KoldPanel::resized()
{
    auto place = [] (Knob* k, int x, int y, int w = 78) { k->setBounds (x, y, w, 84); };
    kalla->setBounds (kW - 470, 16, 160, 24);

    secKurva = { 20, 80, 580, 440 };
    kurvan.setBounds (secKurva.getX() + 6, secKurva.getY() + 30, secKurva.getWidth() - 12, 256);
    {
        const int y = secKurva.getY() + 296;
        form->setBounds (secKurva.getX() + 16, y, 130, 26);
        ducktid->setBounds (secKurva.getX() + 156, y, 80, 26);
        duckmal->setBounds (secKurva.getX() + 246, y, 140, 26);
        const int ky = y + 38;
        place (djup, secKurva.getX() + 20, ky);
        place (kanslighet, secKurva.getX() + 120, ky);
        place (kicklangd, secKurva.getX() + 220, ky);
    }

    const int rx = 616, rw = kW - rx - 20;
    secGilj = { rx, 80, rw, 140 };
    place (subton, rx + 10, 110, 96);
    place (subniva, rx + 104, 110);
    place (subfall, rx + 184, 110);
    lage->setBounds (rx + 266, 136, 70, 24);

    secTrans = { rx, 230, rw / 2 - 5, 124 };
    place (anslag, secTrans.getX() + 6, 262);
    place (svans, secTrans.getX() + 84, 262);

    secUt = { rx + rw / 2 + 5, 230, rw / 2 - 5, 124 };
    place (blandning, secUt.getX() + 6, 262);
    place (niva, secUt.getX() + 84, 262);

    secDist = { rx, 364, rw, 156 };
    place (brytpunkt, rx + 10, 400);
    place (subdist, rx + 96, 400);
    place (ton, rx + 182, 400);
    monoBtn.setBounds (rx + 270, 426, 70, 26);
}

//==============================================================================
KoldEditor::KoldEditor (KoldProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);
    sprak::laddaOm();
    byggPanel();
    sprakKnapp.accent = cold;
    sprakKnapp.onClick = [this]
    {
        sprak::sattEngelska (! sprak::engelska());
        juce::Component::SafePointer<KoldEditor> self (this);
        juce::MessageManager::callAsync ([self] { if (self != nullptr) self->byggPanel(); });
    };
    addAndMakeVisible (sprakKnapp);
    setSize (kW, kH);
}

KoldEditor::~KoldEditor() { setLookAndFeel (nullptr); }

void KoldEditor::byggPanel()
{
    panel.reset();
    panel = std::make_unique<KoldPanel> (proc);
    sprak::oversattAllt (*panel);
    addAndMakeVisible (*panel);
    sprakKnapp.toFront (false);
    if (getWidth() > 0) resized();
}

void KoldEditor::resized()
{
    if (panel != nullptr) panel->setBounds (getLocalBounds());
    sprakKnapp.setBounds (326, 32, 38, 14);
}
