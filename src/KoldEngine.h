// KÖLDBÄRAREN - MASKINHALLEN / Phunkshunz AUDIO
// Botten-maskinen. Ren C++ utan JUCE, så att den kan testas fristående.
//
// Köldbäraren är vätskan som bär kylan genom en anläggning. Här bär den botten under kicken.
//
// Signalkedjan:
//   IN ─┬─ DELNING (Linkwitz-Riley 4:e ordningen, BRYTPUNKT 60–250 Hz)
//       │    BOTTEN: mono ─> SUBDIST (asymmetrisk mättning + tonfilter) ─┐
//       │    TOPP:   orörd ─────────────────────────────────────────────┤
//       │                                                               ├─> TRANSIENT (ANSLAG / SVANS)
//       │                                                               └─> DUCK (ritad kurva, på allt eller bara botten)
//       │                                                                    + GILJOTIN (faslåst sub efter kicken)
//       └─────────────────────────────────── torrt ──────────────> BLANDNING ─> NIVÅ ─> UT
//
// Vad som startar kurvan och giljotinen (KÄLLA):
//   TAKT       varje fjärdedel från värdens klocka. Kicken anses vara KICKLÄNGD lång.
//   SIDECHAIN  kicken på sidokedjan (ingång 2). Giljotinen faller exakt när kicken släpper.
//   SJÄLV      anslagen i signalen själv (botten under 150 Hz).
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace kold
{
constexpr float kPi = 3.14159265358979f;
constexpr int kCurvePoints = 32;   // den ritade kurvan: 32 punkter över DUCKTID

inline float clamp01 (float x) { return x < 0.f ? 0.f : (x > 1.f ? 1.f : x); }
inline float dbToGain (float db) { return std::pow (10.f, db / 20.f); }

// Linjär upp till 0.9, sedan mjukt knä: klipper aldrig över 0 dBFS
inline float softClip (float x)
{
    const float a = std::abs (x);
    if (a <= 0.9f) return x;
    const float y = 0.9f + 0.1f * std::tanh ((a - 0.9f) / 0.1f);
    return x < 0.f ? -y : y;
}

// Topology-preserving state variable filter (Cytomic / Simper)
struct SVF
{
    float ic1 = 0, ic2 = 0, a1 = 0, a2 = 0, a3 = 0, k = 1.414f;
    struct Out { float lp, bp, hp; };
    void set (float fc, float q, float fs)
    {
        fc = std::min (fc, fs * 0.45f);
        const float g = std::tan (kPi * fc / fs);
        k = 1.f / q;
        a1 = 1.f / (1.f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }
    Out tick (float v0)
    {
        const float v3 = v0 - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.f * v1 - ic1;
        ic2 = 2.f * v2 - ic2;
        return { v2, v1, v0 - k * v1 - v2 };
    }
    void reset() { ic1 = ic2 = 0; }
};

// Linkwitz-Riley 4: två Butterworth-SVF i rad. Botten + topp summerar platt i amplitud.
struct LR4
{
    SVF lp1, lp2, hp1, hp2;
    void set (float fc, float fs)
    {
        for (auto* f : { &lp1, &lp2, &hp1, &hp2 }) f->set (fc, 0.70710678f, fs);
    }
    void tick (float x, float& lo, float& hi)
    {
        lo = lp2.tick (lp1.tick (x).lp).lp;
        hi = hp2.tick (hp1.tick (x).hp).hp;
    }
    void reset() { for (auto* f : { &lp1, &lp2, &hp1, &hp2 }) f->reset(); }
};

enum class Source { Takt = 0, Sidechain = 1, Self = 2 };
enum class GuillotineMode { Slap = 0, Spikrakt = 1 };   // SLÄP (techno): svansen får dö ut. SPIKRAKT (psy): kapas kirurgiskt.

struct Params
{
    Source source = Source::Takt;
    // DUCK
    std::array<float, kCurvePoints> curve {};   // 0 = helt nere, 1 = full nivå. Fylls av standardkurvan.
    float duckDepth = 0.7f;      // 0..1
    float duckBeats = 1.0f;      // kurvans längd i fjärdedelar (0.25 = 1/16 ... 2 = 1/2)
    bool duckOnlyLow = true;     // DUCKA: BARA BOTTEN eller ALLT
    float threshold = 0.35f;     // KÄNSLIGHET för SIDECHAIN/SJÄLV, andel av senaste toppen
    float kickMs = 140.f;        // KICKLÄNGD i TAKT-läget
    float gridBeats = 1.f;       // TAKT-läget: hur ofta kurvan startar, i fjärdedelar (1 = 1/4, 0.5 = 1/8)
    float gridOffset = 0.f;      // TAKT-läget: förskjutning i fjärdedelar (0.5 = offbeat)
    // TRANSIENT
    float attack = 0.f;          // ANSLAG -1..+1
    float sustain = 0.f;         // SVANS -1..+1
    // GILJOTIN
    float subHz = 45.f;          // SUBTON 30..90 Hz
    float subLevel = 0.0f;       // SUBNIVÅ 0..1 (av = 0)
    float subDecay = 0.4f;       // SUBFALL 0.05..1.5 s
    GuillotineMode mode = GuillotineMode::Slap;
    // SUBDIST
    float crossoverHz = 120.f;   // BRYTPUNKT
    float drive = 0.f;           // SUBDIST 0..1
    float driveTone = 600.f;     // TON efter mättningen, 150..2000 Hz
    bool monoLow = true;         // botten i mono
    // UT
    float mix = 1.f;
    float outputDb = 0.f;

    Params() { setDefaultCurve (*this); }

    // Klassisk pump: ner direkt, tillbaka med en mjuk böj till halva fjärdedelen
    static void setDefaultCurve (Params& p)
    {
        for (int i = 0; i < kCurvePoints; ++i)
        {
            const float t = (float) i / (float) (kCurvePoints - 1);
            p.curve[(size_t) i] = clamp01 (std::pow (std::min (1.f, t / 0.55f), 1.8f));
        }
    }
};

// Kurvans värde vid t (0..1), linjärt mellan punkterna
inline float curveAt (const std::array<float, kCurvePoints>& c, float t)
{
    if (t >= 1.f) return c[kCurvePoints - 1];
    if (t <= 0.f) return c[0];
    const float x = t * (float) (kCurvePoints - 1);
    const int i = (int) x;
    const float f = x - (float) i;
    return c[(size_t) i] + (c[(size_t) std::min (i + 1, kCurvePoints - 1)] - c[(size_t) i]) * f;
}

//==============================================================================
class Engine
{
public:
    void prepare (double sampleRate)
    {
        fs = (float) sampleRate;
        split[0].reset(); split[1].reset();
        detLp.reset(); toneL.reset(); toneR.reset(); dcL.reset(); dcR.reset();
        applyFilters();
        envFast = envSlow = 0.f;
        detEnv = peakHold = 0.f;
        kickOn = false;
        sinceTrigger = 1.0e9f;
        sinceKickStart = 1.0e9f;
        refractory = 0.f;
        subPhase = 0.f;
        subEnv = 0.f;
        subGate = 0.f;
        curveGain = 1.f;
        smoothGain = 1.f;
        lastBeat = -1.0e9;
    }

    void setParams (const Params& np)
    {
        const bool filtersChanged = np.crossoverHz != p.crossoverHz || np.driveTone != p.driveTone;
        p = np;
        if (filtersChanged && fs > 0.f) applyFilters();
    }

    // Värdens tempo och läge (fjärdedelar sedan start). ppq < 0 = ingen klocka (stoppad).
    void setTransport (double bpmIn, double ppqAtBlockStart, bool playing)
    {
        bpm = std::max (40.0, std::min (300.0, bpmIn));
        ppq = ppqAtBlockStart;
        isPlaying = playing;
    }

    // scL/scR får vara nullptr (ingen sidokedja inkopplad)
    void process (float* L, float* R, const float* scL, const float* scR, int n)
    {
        const float samplesPerBeat = (float) (fs * 60.0 / bpm);
        const float curveLen = std::max (16.f, samplesPerBeat * p.duckBeats);
        const float kickLen = p.kickMs * 0.001f * fs;
        const float out = dbToGain (p.outputDb);
        const float subInc = p.subHz / fs;
        const float subFall = std::exp (-1.f / (fs * std::max (0.02f, p.subDecay)));
        const float ramp = 1.f / (0.0006f * fs);          // giljotinens kant: 0.6 ms, inget knaster
        const float atkCoef = std::exp (-1.f / (0.0008f * fs)), relFast = std::exp (-1.f / (0.02f * fs));
        const float slowCoef = std::exp (-1.f / (0.09f * fs));
        const float detRel = std::exp (-1.f / (0.04f * fs));
        const float peakFall = std::exp (-1.f / (2.0f * fs));
        const float smoothC = std::exp (-1.f / (0.0007f * fs));
        const float driveAmt = 1.f + 14.f * p.drive * p.drive;
        const float driveNorm = 1.f / std::tanh (driveAmt);

        for (int i = 0; i < n; ++i)
        {
            const float dryL = L[i], dryR = R[i];

            // ---------- vad startar kurvan?
            bool trigger = false, kickEnded = false;
            if (p.source == Source::Takt)
            {
                if (isPlaying)
                {
                    const double beatPos = ppq + (double) i / samplesPerBeat;
                    const double grid = (beatPos - (double) p.gridOffset) / (double) std::max (0.0625f, p.gridBeats);
                    const double b = std::floor (grid + 1.0e-9);
                    if (b != lastBeat) { trigger = lastBeat > -1.0e8 || grid - b < 0.01; lastBeat = b; }
                }
                if (trigger) { kickOn = true; sinceKickStart = 0.f; }
                if (kickOn && sinceKickStart >= kickLen) { kickOn = false; kickEnded = true; }
            }
            else
            {
                // detektorn: botten (<150 Hz) av sidokedjan eller signalen själv
                float d = 0.f;
                if (p.source == Source::Sidechain && scL != nullptr)
                    d = 0.5f * (scL[i] + (scR != nullptr ? scR[i] : scL[i]));
                else if (p.source == Source::Self)
                    d = 0.5f * (dryL + dryR);
                d = std::abs (detLp.tick (d).lp);
                detEnv = d > detEnv ? d : detEnv * detRel;
                peakHold = std::max (detEnv, peakHold * peakFall);
                const float on = std::max (1.0e-4f, peakHold * (0.15f + 0.7f * p.threshold));
                refractory = std::max (0.f, refractory - 1.f);
                if (! kickOn && detEnv > on && refractory <= 0.f)
                {
                    kickOn = true; trigger = true; sinceKickStart = 0.f;
                    refractory = 0.06f * fs;   // inte oftare än var 60:e ms
                }
                else if (kickOn && detEnv < on * 0.5f)
                {
                    kickOn = false; kickEnded = true;
                }
            }
            if (trigger) sinceTrigger = 0.f;
            sinceTrigger += 1.f;
            sinceKickStart += 1.f;

            // ---------- DELNING och SUBDIST
            float loL, hiL, loR, hiR;
            split[0].tick (dryL, loL, hiL);
            split[1].tick (dryR, loR, hiR);
            if (p.monoLow) { const float m = 0.5f * (loL + loR); loL = loR = m; }
            if (p.drive > 0.001f)
            {
                auto sat = [&] (float x, SVF& tone, SVF& dc)
                {
                    // asymmetrisk: jämna övertoner, låter som en överstyrd bashögtalare, inte som en fuzz
                    const float y = std::tanh ((x + 0.12f * p.drive) * driveAmt) * driveNorm - std::tanh (0.12f * p.drive * driveAmt) * driveNorm;
                    const float t = tone.tick (y).lp;
                    const float h = dc.tick (t).hp;   // bort med likström
                    return x + (h * (0.9f / (0.6f + p.drive)) - x) * std::min (1.f, p.drive * 2.f);
                };
                loL = sat (loL, toneL, dcL);
                loR = p.monoLow ? loL : sat (loR, toneR, dcR);
            }

            // ---------- TRANSIENT: snabb och långsam följare, skillnaden är anslaget
            const float lvl = std::abs (loL + hiL) + std::abs (loR + hiR);
            envFast = lvl > envFast ? lvl + (envFast - lvl) * atkCoef : lvl + (envFast - lvl) * relFast;
            envSlow = lvl + (envSlow - lvl) * slowCoef;
            float shape = 1.f;
            if (p.attack != 0.f || p.sustain != 0.f)
            {
                const float tr = envFast > 1.0e-6f ? clamp01 ((envFast - envSlow) / envFast) : 0.f;   // 0..1: hur mycket anslag just nu
                const float g = 1.f + p.attack * 2.0f * tr + p.sustain * 0.9f * (1.f - tr);
                shape = std::max (0.f, std::min (3.f, g));
            }

            // ---------- DUCK
            const float t = sinceTrigger / curveLen;
            const float c = t < 1.f ? curveAt (p.curve, t) : 1.f;
            curveGain = 1.f - p.duckDepth * (1.f - c);
            smoothGain = curveGain + (smoothGain - curveGain) * smoothC;
            const float duckLo = smoothGain;
            const float duckHi = p.duckOnlyLow ? 1.f : smoothGain;

            float wetL = (loL * duckLo + hiL * duckHi) * shape;
            float wetR = (loR * duckLo + hiR * duckHi) * shape;

            // ---------- GILJOTIN: sub med faslåst start när kicken släpper, död så fort nästa kick kommer
            if (p.subLevel > 0.0001f)
            {
                if (kickEnded) { subPhase = 0.f; subEnv = 1.f; }
                if (kickOn) subGate = std::max (0.f, subGate - ramp);
                else if (subEnv > 0.f) subGate = std::min (1.f, subGate + ramp * 4.f);
                float s = 0.f;
                if (subEnv > 1.0e-4f || subGate > 0.f)
                {
                    s = std::sin (2.f * kPi * subPhase) * subEnv * subGate;
                    subPhase += subInc;
                    if (subPhase >= 1.f) subPhase -= 1.f;
                    if (! kickOn)
                    {
                        subEnv *= subFall;
                        if (p.mode == GuillotineMode::Spikrakt && subEnv < 0.15f) subEnv *= 0.995f;   // kirurgisk amputering
                    }
                }
                if (kickOn && subGate <= 0.f) subEnv = 0.f;
                // suben följer ljudet: tystnar ljudet in (breakdown) tystnar suben också
                subOut = s * p.subLevel * 0.8f * clamp01 (envSlow * 12.f);
                wetL += subOut;
                wetR += subOut;
            }
            else subOut = 0.f;

            // ---------- BLANDNING och NIVÅ
            L[i] = softClip ((dryL + (wetL - dryL) * p.mix) * out);
            R[i] = softClip ((dryR + (wetR - dryR) * p.mix) * out);

            meterGain = smoothGain;
            meterSub = std::max (std::abs (subOut), meterSub * 0.9995f);
        }
        if (p.source == Source::Takt) ppq += (double) n / samplesPerBeat;
        kickFlag = kickOn;
    }

    // för panelen
    float duckGain() const { return meterGain; }
    float subLevelNow() const { return meterSub; }
    bool kickActive() const { return kickFlag; }
    float curvePosition() const   // 0..1 var i kurvan vi är, >1 = efter
    {
        const float spb = (float) (fs * 60.0 / bpm);
        return sinceTrigger / std::max (16.f, spb * p.duckBeats);
    }

private:
    void applyFilters()
    {
        split[0].set (p.crossoverHz, fs);
        split[1].set (p.crossoverHz, fs);
        detLp.set (150.f, 0.7f, fs);
        toneL.set (p.driveTone, 0.6f, fs);
        toneR.set (p.driveTone, 0.6f, fs);
        dcL.set (18.f, 0.7f, fs);
        dcR.set (18.f, 0.7f, fs);
    }

    Params p;
    float fs = 48000.f;
    double bpm = 124.0, ppq = 0.0, lastBeat = -1.0e9;
    bool isPlaying = false;

    LR4 split[2];
    SVF detLp, toneL, toneR, dcL, dcR;
    float envFast = 0, envSlow = 0;
    float detEnv = 0, peakHold = 0, refractory = 0;
    bool kickOn = false, kickFlag = false;
    float sinceTrigger = 1.0e9f, sinceKickStart = 1.0e9f;
    float subPhase = 0, subEnv = 0, subGate = 0, subOut = 0;
    float curveGain = 1.f, smoothGain = 1.f;
    float meterGain = 1.f, meterSub = 0.f;
};

} // namespace kold
