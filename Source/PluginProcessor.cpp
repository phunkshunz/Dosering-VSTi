#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
// Kurvans former, 32 punkter över DUCKTID. 0 = helt nere, 1 = full nivå.
void Former::fill (int form, std::array<float, kold::kCurvePoints>& c)
{
    const int N = kold::kCurvePoints;
    for (int i = 0; i < N; ++i)
    {
        const float t = (float) i / (float) (N - 1);
        float v = 1.f;
        switch (form)
        {
            case 0: v = std::pow (std::min (1.f, t / 0.55f), 1.8f); break;                  // PUMP: klassisk
            case 1: v = t < 0.28f ? 0.f : std::min (1.f, (t - 0.28f) / 0.08f); break;         // HÅRD: nere, sen rakt upp
            case 2: v = 0.5f - 0.5f * std::cos (kold::kPi * std::min (1.f, t / 0.85f)); break; // MJUK: lång cosinus
            case 3: v = std::floor (std::min (1.f, t / 0.75f) * 4.f) / 4.f; break;           // TRAPPA: fyra steg upp
            case 4: v = std::pow (std::min (1.f, t), 0.5f); break;                           // ANDNING: snabbt upp, långsamt sista biten
            case 5: { const float h = t < 0.5f ? t * 2.f : (t - 0.5f) * 2.f;                  // DUBBEL: två pumpar per slag
                      v = std::pow (std::min (1.f, h / 0.6f), 1.6f); } break;
            case 6: v = t < 0.45f ? 0.f : 1.f; break;                                        // GRIND: av/på
            default: v = c[(size_t) i]; break;                                               // EGEN: rör inte
        }
        c[(size_t) i] = kold::clamp01 (v);
    }
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout KoldProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;
    auto attr = [] (std::function<String (float)> fn)
    { return AudioParameterFloatAttributes().withStringFromValueFunction ([fn] (float v, int) { return fn (v); }); };
    auto pct = attr ([] (float v) { return String (roundToInt (v * 100.f)) + " %"; });
    auto bip = attr ([] (float v) { return (v > 0.005f ? "+" : "") + String (roundToInt (v * 100.f)) + " %"; });
    auto skew = [] (float lo, float hi, float centre) { NormalisableRange<float> r (lo, hi); r.setSkewForCentre (centre); return r; };
    auto hz = attr ([] (float v) { return String (roundToInt (v)) + " Hz"; });
    auto note = attr ([] (float v)
    {
        static const char* n[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        const int m = roundToInt (12.f * std::log2 (v / 440.f) + 69.f);
        return String (v, 1) + " Hz  " + n[(m % 12 + 12) % 12] + String (m / 12 - 1);
    });

    // DUCK
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::kalla, 1 }, String::fromUTF8 ("Källa"),
                     StringArray { "TAKT", "SIDECHAIN", String::fromUTF8 ("SJÄLV") }, 0));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::form, 1 }, "Form", Former::names(), 0));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::djup, 1 }, "Djup", NormalisableRange<float> (0.f, 1.f), 0.7f, pct));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::ducktid, 1 }, "Ducktid", StringArray { "1/16", "1/8", "1/4", "1/2" }, 2));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::duckmal, 1 }, "Ducka", StringArray { "BARA BOTTEN", "ALLT" }, 0));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::kanslighet, 1 }, String::fromUTF8 ("Känslighet"), NormalisableRange<float> (0.f, 1.f), 0.65f, pct));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::kicklangd, 1 }, String::fromUTF8 ("Kicklängd"), skew (40.f, 400.f, 140.f), 140.f,
                     attr ([] (float v) { return String (roundToInt (v)) + " ms"; })));
    // TRANSIENT
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::anslag, 1 }, "Anslag", NormalisableRange<float> (-1.f, 1.f), 0.f, bip));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::svans, 1 }, "Svans", NormalisableRange<float> (-1.f, 1.f), 0.f, bip));
    // GILJOTIN
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::subton, 1 }, "Subton", skew (30.f, 90.f, 50.f), 45.f, note));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::subniva, 1 }, String::fromUTF8 ("Subnivå"), NormalisableRange<float> (0.f, 1.f), 0.f, pct));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::subfall, 1 }, "Subfall", skew (0.05f, 1.5f, 0.4f), 0.4f,
                     attr ([] (float v) { return String (roundToInt (v * 1000.f)) + " ms"; })));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::lage, 1 }, String::fromUTF8 ("Läge"),
                     StringArray { String::fromUTF8 ("SLÄP"), "SPIKRAKT" }, 0));
    // SUBDIST
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::brytpunkt, 1 }, "Brytpunkt", skew (60.f, 250.f, 120.f), 120.f, hz));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::subdist, 1 }, "Subdist", NormalisableRange<float> (0.f, 1.f), 0.f, pct));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::ton, 1 }, "Ton", skew (150.f, 2000.f, 600.f), 600.f, hz));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::monobas, 1 }, "Mono bas", true));
    // UT
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::blandning, 1 }, "Blandning", NormalisableRange<float> (0.f, 1.f), 1.f, pct));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::niva, 1 }, String::fromUTF8 ("Nivå"),
                     NormalisableRange<float> (-24.f, 6.f, 0.1f), 0.f, AudioParameterFloatAttributes().withLabel ("dB")));
    return { p.begin(), p.end() };
}

//==============================================================================
KoldProcessor::KoldProcessor()
    : AudioProcessor (BusesProperties().withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                                       .withInput  ("Sidechain", juce::AudioChannelSet::stereo(), false)),
      apvts (*this, nullptr, "KOLDBARAREN", createLayout())
{
    applyForm (0);
    apvts.addParameterListener (ParamIDs::form, this);
}

KoldProcessor::~KoldProcessor() { apvts.removeParameterListener (ParamIDs::form, this); }

void KoldProcessor::parameterChanged (const juce::String& id, float value)
{
    if (id == ParamIDs::form && ! drawing) applyForm (juce::roundToInt (value));
}

void KoldProcessor::applyForm (int form)
{
    if (form == Former::kEgen) return;
    std::array<float, kold::kCurvePoints> c {};
    Former::fill (form, c);
    for (int i = 0; i < kold::kCurvePoints; ++i) curve[(size_t) i].store (c[(size_t) i]);
}

void KoldProcessor::setCurvePoint (int i, float v)
{
    if (! juce::isPositiveAndBelow (i, kold::kCurvePoints)) return;
    curve[(size_t) i].store (kold::clamp01 (v));
    if (juce::roundToInt (apvts.getRawParameterValue (ParamIDs::form)->load()) != Former::kEgen)
        if (auto* prm = apvts.getParameter (ParamIDs::form))
        {
            drawing = true;
            prm->setValueNotifyingHost (prm->convertTo0to1 ((float) Former::kEgen));
            drawing = false;
        }
}

bool KoldProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    const auto in  = layouts.getMainInputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo()) return false;
    if (in != juce::AudioChannelSet::mono() && in != juce::AudioChannelSet::stereo()) return false;
    if (layouts.inputBuses.size() > 1)
    {
        const auto sc = layouts.getChannelSet (true, 1);
        if (! sc.isDisabled() && sc != juce::AudioChannelSet::mono() && sc != juce::AudioChannelSet::stereo()) return false;
    }
    return true;
}

kold::Params KoldProcessor::readParams() const
{
    auto v = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    auto i = [&v] (const char* id) { return juce::roundToInt (v (id)); };
    kold::Params p;
    p.source = (kold::Source) juce::jlimit (0, 2, i (ParamIDs::kalla));
    for (int k = 0; k < kold::kCurvePoints; ++k) p.curve[(size_t) k] = curve[(size_t) k].load (std::memory_order_relaxed);
    p.duckDepth = v (ParamIDs::djup);
    static const float beats[] = { 0.25f, 0.5f, 1.f, 2.f };
    p.duckBeats = beats[juce::jlimit (0, 3, i (ParamIDs::ducktid))];
    p.duckOnlyLow = i (ParamIDs::duckmal) == 0;
    p.threshold = 1.f - v (ParamIDs::kanslighet);   // högre känslighet = lägre tröskel
    p.kickMs = v (ParamIDs::kicklangd);
    p.attack = v (ParamIDs::anslag);
    p.sustain = v (ParamIDs::svans);
    p.subHz = v (ParamIDs::subton);
    p.subLevel = v (ParamIDs::subniva);
    p.subDecay = v (ParamIDs::subfall);
    p.mode = (kold::GuillotineMode) juce::jlimit (0, 1, i (ParamIDs::lage));
    p.crossoverHz = v (ParamIDs::brytpunkt);
    p.drive = v (ParamIDs::subdist);
    p.driveTone = v (ParamIDs::ton);
    p.monoLow = v (ParamIDs::monobas) > 0.5f;
    p.mix = v (ParamIDs::blandning);
    p.outputDb = v (ParamIDs::niva);
    return p;
}

void KoldProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.setParams (readParams());
    engine.prepare (sampleRate);
    bufL.assign ((size_t) juce::jmax (1, samplesPerBlock), 0.f);
    bufR.assign ((size_t) juce::jmax (1, samplesPerBlock), 0.f);
}

void KoldProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    midi.clear();
    const int n = buffer.getNumSamples();
    if (n <= 0) return;
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
    engine.setParams (readParams());
    engine.setTransport (bpm, ppq, playing);

    auto main = getBusBuffer (buffer, true, 0);
    const int ins = main.getNumChannels();
    if (ins <= 0) return;
    std::copy (main.getReadPointer (0), main.getReadPointer (0) + n, bufL.begin());
    std::copy (main.getReadPointer (ins > 1 ? 1 : 0), main.getReadPointer (ins > 1 ? 1 : 0) + n, bufR.begin());

    const float* scL = nullptr;
    const float* scR = nullptr;
    if (getBusCount (true) > 1)
        if (auto* bus = getBus (true, 1); bus != nullptr && bus->isEnabled() && bus->getNumberOfChannels() > 0)
        {
            auto sc = getBusBuffer (buffer, true, 1);
            scL = sc.getReadPointer (0);
            scR = sc.getReadPointer (sc.getNumChannels() > 1 ? 1 : 0);
        }
    sidechainConnected.store (scL != nullptr);

    engine.process (bufL.data(), bufR.data(), scL, scR, n);

    auto out = getBusBuffer (buffer, false, 0);
    if (out.getNumChannels() >= 2)
    {
        out.copyFrom (0, 0, bufL.data(), n);
        out.copyFrom (1, 0, bufR.data(), n);
    }
    else if (out.getNumChannels() == 1)
    {
        out.copyFrom (0, 0, bufL.data(), n, 0.5f);
        out.addFrom  (0, 0, bufR.data(), n, 0.5f);
    }
    meterDuck.store (engine.duckGain());
    meterSub.store (engine.subLevelNow());
    meterKick.store (engine.kickActive());
    curvePos.store (engine.curvePosition());
}

//==============================================================================
void KoldProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    juce::String c;
    for (int k = 0; k < kold::kCurvePoints; ++k) c << juce::String (curve[(size_t) k].load(), 3) << (k + 1 < kold::kCurvePoints ? " " : "");
    state.setProperty ("kurva", c, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void KoldProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (isDemo) return;
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            const auto c = juce::StringArray::fromTokens (tree.getProperty ("kurva").toString(), " ", "");
            drawing = true;   // replaceState får inte skriva över en EGEN kurva med en form
            apvts.replaceState (tree);
            drawing = false;
            if (c.size() == kold::kCurvePoints)
                for (int k = 0; k < kold::kCurvePoints; ++k) curve[(size_t) k].store (kold::clamp01 (c[k].getFloatValue()));
            else
                applyForm (juce::roundToInt (apvts.getRawParameterValue (ParamIDs::form)->load()));
        }
}

juce::AudioProcessorEditor* KoldProcessor::createEditor() { return new KoldEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new KoldProcessor(); }
