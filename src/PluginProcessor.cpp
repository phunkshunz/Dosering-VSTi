#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

namespace
{
// ordningen här är ordningen i v[]
const char* const kIds[19] = { "mangd", "lage", "mix", "ut", "allt", "duck", "sub", "dist", "anslag", "svans",
                               "langd", "form", "hall", "subhz", "subfall", "bryt", "ton", "spik", "reserv" };
enum { iMangd, iLage, iMix, iUt, iAllt, iDuck, iSub, iDist, iAnslag, iSvans, iLangd, iForm, iHall, iSubHz, iSubFall, iBryt, iTon, iSpik };
}

AudioProcessorValueTreeState::ParameterLayout DoseringProcessor::layout()
{
    AudioProcessorValueTreeState::ParameterLayout l;
    auto f = [&l] (const char* id, const char* name, float lo, float hi, float step, float def)
    {
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, String::fromUTF8 (name), NormalisableRange<float> (lo, hi, step), def));
    };
    f ("mangd", "MÄNGD", 0.0f, 1.0f, 0.001f, 0.5f);
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "lage", 1 }, String::fromUTF8 ("LÄGE"),
                                                   StringArray { "1/4", "1/8", "OFFBEAT", "LYSSNA" }, 0));
    f ("mix", "MIX", 0.0f, 1.0f, 0.001f, 1.0f);
    f ("ut", "UT", -12.0f, 12.0f, 0.1f, 0.0f);
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "allt", 1 }, "PUMPA ALLT", false));
    // karaktären: det presets och SLUMPA ställer in. Syns inte på panelen men går att automatisera.
    f ("duck", "KARAKTÄR DUCK", 0.0f, 1.0f, 0.001f, 0.85f);
    f ("sub", "KARAKTÄR SUB", 0.0f, 1.0f, 0.001f, 0.5f);
    f ("dist", "KARAKTÄR RIV", 0.0f, 1.0f, 0.001f, 0.3f);
    f ("anslag", "KARAKTÄR ANSLAG", -1.0f, 1.0f, 0.01f, 0.0f);
    f ("svans", "KARAKTÄR SVANS", -1.0f, 1.0f, 0.01f, 0.0f);
    f ("langd", "KARAKTÄR LÄNGD", 0.1f, 1.0f, 0.01f, 0.6f);
    f ("form", "KARAKTÄR FORM", 0.3f, 3.0f, 0.01f, 1.8f);
    f ("hall", "KARAKTÄR HÅLL", 0.0f, 0.5f, 0.01f, 0.05f);
    f ("subhz", "KARAKTÄR SUBTON", 30.0f, 90.0f, 0.1f, 45.0f);
    f ("subfall", "KARAKTÄR SUBFALL", 0.05f, 1.5f, 0.01f, 0.4f);
    f ("bryt", "KARAKTÄR BRYTPUNKT", 60.0f, 250.0f, 1.0f, 120.0f);
    f ("ton", "KARAKTÄR TON", 150.0f, 2000.0f, 1.0f, 600.0f);
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "spik", 1 }, String::fromUTF8 ("KARAKTÄR SPIKRAKT"), false));
    return l;
}

DoseringProcessor::DoseringProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "DOSERING", layout())
{
    for (int i = 0; i <= iSpik; ++i) v[i] = apvts.getRawParameterValue (kIds[i]);
    setCurrentProgram (0);
}

bool DoseringProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != AudioChannelSet::mono() && out != AudioChannelSet::stereo()) return false;
    return layouts.getMainInputChannelSet() == out;
}

void DoseringProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate);
    bufL.assign ((size_t) jmax (16, samplesPerBlock), 0.0f);
    bufR.assign ((size_t) jmax (16, samplesPerBlock), 0.0f);
    holdDuck = holdSub = 0.0f;
}

dos::Settings DoseringProcessor::read() const
{
    dos::Settings s;
    s.amount = v[iMangd]->load();
    s.mode = jlimit (0, 3, roundToInt (v[iLage]->load()));
    s.mix = v[iMix]->load();
    s.outDb = v[iUt]->load();
    s.allt = v[iAllt]->load() > 0.5f;
    s.duck = v[iDuck]->load(); s.sub = v[iSub]->load(); s.dist = v[iDist]->load();
    s.anslag = v[iAnslag]->load(); s.svans = v[iSvans]->load();
    s.langd = v[iLangd]->load(); s.form = v[iForm]->load(); s.hall = v[iHall]->load();
    s.subHz = v[iSubHz]->load(); s.subFall = v[iSubFall]->load();
    s.brytHz = v[iBryt]->load(); s.tonHz = v[iTon]->load();
    s.spik = v[iSpik]->load() > 0.5f;
    return s;
}

void DoseringProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer&)
{
    ScopedNoDenormals noDenormals;
    const int numCh = jmin (2, buffer.getNumChannels());
    const int n = buffer.getNumSamples();
    if (numCh == 0 || n == 0) return;
    for (int c = getTotalNumInputChannels(); c < buffer.getNumChannels(); ++c) buffer.clear (c, 0, n);
    if ((int) bufL.size() < n) { bufL.resize ((size_t) n); bufR.resize ((size_t) n); }

    double bpm = 124.0, ppq = 0.0;
    bool playing = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = *b;
            if (auto q = pos->getPpqPosition()) ppq = *q;
            playing = pos->getIsPlaying();
        }
    hostPlaying.store (playing);

    const auto s = read();
    const auto p = dos::toParams (s);
    engine.setParams (p);
    engine.setTransport (bpm, ppq, playing);

    // alltid stereo inuti: mono in läggs på båda sidor
    std::copy (buffer.getReadPointer (0), buffer.getReadPointer (0) + n, bufL.begin());
    std::copy (buffer.getReadPointer (numCh > 1 ? 1 : 0), buffer.getReadPointer (numCh > 1 ? 1 : 0) + n, bufR.begin());
    float peakIn = 0.0f;
    for (int i = 0; i < n; ++i) peakIn = jmax (peakIn, std::abs (bufL[(size_t) i]));

    engine.process (bufL.data(), bufR.data(), nullptr, nullptr, n);

    if (numCh > 1)
    {
        buffer.copyFrom (0, 0, bufL.data(), n);
        buffer.copyFrom (1, 0, bufR.data(), n);
    }
    else
    {
        buffer.copyFrom (0, 0, bufL.data(), n, 0.5f);
        buffer.addFrom (0, 0, bufR.data(), n, 0.5f);
    }

    // lamporna: djupaste ducken och starkaste suben sedan sist, faller mjukt
    const float fall = std::exp (-(float) n / (0.12f * (float) getSampleRate()));
    holdDuck = jmax (1.0f - engine.duckGain(), holdDuck * fall);
    holdSub  = jmax (engine.subLevelNow(), holdSub * fall);
    mDuck.store (p.mix > 0.01f ? holdDuck : 0.0f);
    mSub.store (p.mix > 0.01f ? holdSub : 0.0f);
    mDist.store (peakIn > 0.003f && p.mix > 0.01f ? p.drive : 0.0f);
}

//==============================================================================
void DoseringProcessor::setParam (const char* id, float value)
{
    if (auto* p = apvts.getParameter (id))
        p->setValueNotifyingHost (p->convertTo0to1 (value));
}

void DoseringProcessor::setCurrentProgram (int index)
{
    index = jlimit (0, dos::kNumPresets - 1, index);
    preset.store (index);
    const auto& p = dos::kPresets[index];
    setParam ("mangd", p.amount);
    setParam ("lage", (float) p.mode);
    setParam ("duck", p.duck); setParam ("sub", p.sub); setParam ("dist", p.dist);
    setParam ("anslag", p.anslag); setParam ("svans", p.svans);
    setParam ("langd", p.langd); setParam ("form", p.form); setParam ("hall", p.hall);
    setParam ("subhz", p.subHz); setParam ("subfall", p.subFall);
    setParam ("bryt", p.brytHz); setParam ("ton", p.tonHz);
    setParam ("allt", p.allt ? 1.0f : 0.0f);
    setParam ("spik", p.spik ? 1.0f : 0.0f);
    setParam ("mix", p.mix);
}

const String DoseringProcessor::getProgramName (int index)
{
    return String::fromUTF8 (dos::kPresets[jlimit (0, dos::kNumPresets - 1, index)].name);
}

void DoseringProcessor::slumpa()
{
    auto& r = Random::getSystemRandom();
    auto mellan = [&r] (float lo, float hi) { return lo + (hi - lo) * r.nextFloat(); };
    setParam ("duck", mellan (0.5f, 1.0f));
    setParam ("sub", r.nextFloat() < 0.25f ? 0.0f : mellan (0.2f, 1.0f));
    setParam ("dist", mellan (0.0f, 0.9f));
    setParam ("anslag", mellan (-0.2f, 0.7f));
    setParam ("svans", mellan (-0.5f, 0.6f));
    setParam ("langd", mellan (0.35f, 0.95f));
    setParam ("form", mellan (1.0f, 2.8f));
    setParam ("hall", mellan (0.0f, 0.18f));
    setParam ("subhz", mellan (36.0f, 56.0f));
    setParam ("subfall", mellan (0.15f, 0.9f));
    setParam ("bryt", mellan (90.0f, 160.0f));
    setParam ("ton", mellan (350.0f, 1500.0f));
    setParam ("spik", r.nextFloat() < 0.3f ? 1.0f : 0.0f);
}

void DoseringProcessor::getStateInformation (MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("preset", preset.load(), nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, destData);
}

void DoseringProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto state = ValueTree::fromXml (*xml);
            preset.store (jlimit (0, dos::kNumPresets - 1, (int) state.getProperty ("preset", 0)));
            apvts.replaceState (state);
        }
}

AudioProcessorEditor* DoseringProcessor::createEditor() { return new DoseringEditor (*this); }

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new DoseringProcessor(); }
