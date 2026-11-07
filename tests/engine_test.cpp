// Fristående test av KÖLDBÄRAREN-motorn (ingen JUCE). Bygg: clang++ -O2 -std=c++17 engine_test.cpp
// Kör med "./engine_test wav" för att även skriva ljudprov.
#include "../Source/KoldEngine.h"
#include <complex>
#include <cstdio>
#include <cstring>
#include <random>
#include <string>

using namespace kold;
static int failures = 0;
#define CHECK(c, msg) do { if (!(c)) { std::printf ("FAIL: %s\n", msg); ++failures; } else std::printf ("ok:   %s\n", msg); } while (0)

constexpr int FS = 48000;
struct Stereo { std::vector<float> l, r; };

static float rms (const std::vector<float>& v, size_t a, size_t b)
{ double s = 0; size_t n = 0; for (size_t i = a; i < b && i < v.size(); ++i) { s += v[i] * v[i]; ++n; } return n ? (float) std::sqrt (s / n) : 0.f; }
static float peak (const std::vector<float>& v, size_t a, size_t b)
{ float p = 0; for (size_t i = a; i < b && i < v.size(); ++i) p = std::max (p, std::abs (v[i])); return p; }
static bool finite (const Stereo& s) { for (float x : s.l) if (! std::isfinite (x)) return false; for (float x : s.r) if (! std::isfinite (x)) return false; return true; }
static float mag (const std::vector<float>& v, size_t a, size_t b, float f)
{
    std::complex<double> acc = 0; size_t n = 0;
    for (size_t i = a; i < b; ++i, ++n) acc += (double) v[i] * std::polar (1.0, -2.0 * M_PI * f * (double) n / FS);
    return (float) (2.0 * std::abs (acc) / (double) std::max<size_t> (1, n));
}
static void writeWav (const std::string& path, const Stereo& s)
{
    FILE* f = std::fopen (path.c_str(), "wb"); if (! f) return;
    int32_t frames = (int32_t) s.l.size(), dataBytes = frames * 4, chunk = 36 + dataBytes, sr = FS, br = FS * 4, sub = 16;
    int16_t fmt = 1, ch = 2, ba = 4, bits = 16;
    std::fwrite ("RIFF", 1, 4, f); std::fwrite (&chunk, 4, 1, f); std::fwrite ("WAVEfmt ", 1, 8, f);
    std::fwrite (&sub, 4, 1, f); std::fwrite (&fmt, 2, 1, f); std::fwrite (&ch, 2, 1, f);
    std::fwrite (&sr, 4, 1, f); std::fwrite (&br, 4, 1, f); std::fwrite (&ba, 2, 1, f); std::fwrite (&bits, 2, 1, f);
    std::fwrite ("data", 1, 4, f); std::fwrite (&dataBytes, 4, 1, f);
    for (int32_t i = 0; i < frames; ++i)
        for (float x : { s.l[(size_t) i], s.r[(size_t) i] })
        { int16_t v = (int16_t) std::lround (std::max (-1.f, std::min (1.f, x)) * 32767.f); std::fwrite (&v, 2, 1, f); }
    std::fclose (f);
}

// Kör motorn i block, med värdens klocka (124 BPM från takt 0) och valfri sidokedja
static Stereo run (const Params& p, Stereo in, const Stereo* sc = nullptr, int block = 256, double bpm = 124.0, Engine* keep = nullptr)
{
    Engine local; Engine& e = keep ? *keep : local;
    e.setParams (p); e.prepare (FS); e.setParams (p);
    const double spb = FS * 60.0 / bpm;
    for (size_t pos = 0; pos < in.l.size(); pos += (size_t) block)
    {
        const int n = (int) std::min ((size_t) block, in.l.size() - pos);
        e.setTransport (bpm, (double) pos / spb, true);
        e.process (in.l.data() + pos, in.r.data() + pos, sc ? sc->l.data() + pos : nullptr, sc ? sc->r.data() + pos : nullptr, n);
    }
    return in;
}

static Stereo sines (size_t len, std::initializer_list<std::pair<float, float>> parts)
{
    Stereo s { std::vector<float> (len), std::vector<float> (len) };
    for (size_t i = 0; i < len; ++i)
    {
        float v = 0; for (auto [f, a] : parts) v += a * std::sin (2.f * kPi * f * (float) i / FS);
        s.l[i] = s.r[i] = v;
    }
    return s;
}

// En techno-kick: sinus 150 -> 48 Hz, 180 ms
static void addKick (Stereo& s, size_t at, float amp = 0.9f, float lenS = 0.18f)
{
    double ph = 0;
    const size_t n = (size_t) (lenS * FS);
    for (size_t i = 0; i < n && at + i < s.l.size(); ++i)
    {
        const float t = (float) i / FS;
        const float f = 48.f + 102.f * std::exp (-t / 0.03f);
        ph += f / FS;
        const float env = std::exp (-t / (lenS * 0.35f)) * std::min (1.f, t / 0.001f) * (i + 200 < n ? 1.f : (float) (n - i) / 200.f);
        const float v = amp * env * (float) std::sin (2.0 * M_PI * ph);
        s.l[at + i] += v; s.r[at + i] += v;
    }
}

int main (int argc, char** argv)
{
    const bool wav = argc > 1 && std::strcmp (argv[1], "wav") == 0;
    const size_t beat = (size_t) (FS * 60.0 / 124.0);

    // 1. Neutral: allt av, ut ≈ in (delningen summerar platt)
    {
        Params p; p.duckDepth = 0.f; p.monoLow = false;
        auto in = sines (FS, { { 55.f, 0.3f }, { 440.f, 0.2f }, { 3000.f, 0.1f } });
        auto out = run (p, in);
        const float a = rms (in.l, FS / 2, FS), b = rms (out.l, FS / 2, FS);
        CHECK (std::abs (20.f * std::log10 (b / a)) < 0.3f, "neutral: samma nivå ut som in (inom 0.3 dB)");
    }

    // 2. TAKT-duck på botten: 60 Hz dyker på slaget, 2 kHz orörd, tillbaka efter halva slaget
    {
        Params p; p.source = Source::Takt; p.duckDepth = 0.8f; p.duckOnlyLow = true;
        auto in = sines ((size_t) (beat * 4), { { 60.f, 0.4f }, { 2000.f, 0.2f } });
        auto out = run (p, in);
        const size_t b2 = beat * 2;
        const float lowAtBeat = mag (out.l, b2 + 200, b2 + 200 + 2400, 60.f);
        const float lowLate = mag (out.l, b2 + (size_t) (beat * 0.7), b2 + (size_t) (beat * 0.7) + 2400, 60.f);
        const float hiAtBeat = mag (out.l, b2 + 200, b2 + 200 + 2400, 2000.f);
        std::printf ("      60 Hz på slaget %.3f, sent %.3f, 2 kHz på slaget %.3f\n", lowAtBeat, lowLate, hiAtBeat);
        CHECK (lowAtBeat < lowLate * 0.45f, "TAKT: botten duckar på slaget");
        CHECK (lowLate > 0.3f, "TAKT: botten är tillbaka efter halva slaget");
        CHECK (hiAtBeat > 0.17f, "BARA BOTTEN: toppen orörd");
        p.duckOnlyLow = false;
        auto out2 = run (p, in);
        CHECK (mag (out2.l, b2 + 200, b2 + 2600, 2000.f) < 0.1f, "ALLT: toppen duckar också");
        if (wav) writeWav ("kold_takt.wav", out);
    }

    // 3. SIDECHAIN + GILJOTIN: kickar på sidokedjan, suben börjar i fas när kicken släpper och tiger under kicken
    {
        Params p; p.source = Source::Sidechain; p.duckDepth = 0.f; p.subLevel = 1.f; p.subHz = 45.f; p.subDecay = 0.5f;
        const size_t len = beat * 8;
        Stereo sc { std::vector<float> (len), std::vector<float> (len) };
        for (int k = 0; k < 8; ++k) addKick (sc, (size_t) k * beat);
        Stereo in { std::vector<float> (len), std::vector<float> (len) };   // tyst huvudingång: bara suben hörs
        Engine e;
        auto out = run (p, in, &sc, 128, 124.0, &e);
        // under kickens första 40 ms (efter första kicken): ingen sub
        const float during = peak (out.l, beat * 3 + (size_t) (0.006f * FS), beat * 3 + (size_t) (0.04f * FS));
        // första sampel där suben hörs efter kick 3
        size_t start = 0;
        for (size_t i = beat * 3 + (size_t) (0.02f * FS); i < beat * 4; ++i) if (std::abs (out.l[i]) > 1.0e-4f) { start = i; break; }
        const float after = rms (out.l, start + 200, start + 4800);
        std::printf ("      under kicken %.4f, sub efter %.3f, startar %.1f ms efter slaget\n", during, after, (start - beat * 3) * 1000.f / FS);
        CHECK (during < 0.01f, "GILJOTIN: ingen sub under kicken (6 ms efter slaget och framåt)");
        CHECK (after > 0.2f, "GILJOTIN: suben kommer när kicken släpper");
        CHECK (start > beat * 3 + (size_t) (0.05f * FS) && start < beat * 3 + (size_t) (0.3f * FS), "GILJOTIN: startar mellan 50 och 300 ms efter slaget");
        // fasen: första halvperioden ska gå uppåt från noll (sinus från fas 0)
        CHECK (out.l[start + 20] > 0.f && std::abs (out.l[start]) < 0.05f, "GILJOTIN: startar i fas (från noll, uppåt)");
        // och den dör innan nästa slag
        CHECK (peak (out.l, beat * 4 + (size_t) (0.006f * FS), beat * 4 + (size_t) (0.03f * FS)) < 0.01f, "GILJOTIN: kapas när nästa kick kommer");
        if (wav) { Stereo mixd = sc; for (size_t i = 0; i < len; ++i) { mixd.l[i] += out.l[i]; mixd.r[i] += out.r[i]; } writeWav ("kold_giljotin.wav", mixd); }
    }

    // 4. SJÄLV: kicken i signalen startar kurvan
    {
        Params p; p.source = Source::Self; p.duckDepth = 0.9f; p.duckOnlyLow = false; p.duckBeats = 1.f;
        const size_t len = beat * 6;
        auto in = sines (len, { { 3000.f, 0.15f } });
        for (int k = 0; k < 6; ++k) addKick (in, (size_t) k * beat);
        auto out = run (p, in);
        const float hiDuck = mag (out.l, beat * 3 + 2400, beat * 3 + 4800, 3000.f);
        const float hiFree = mag (out.l, beat * 3 + (size_t) (beat * 0.8), beat * 3 + (size_t) (beat * 0.8) + 2400, 3000.f);
        std::printf ("      3 kHz efter kicken %.3f, sent %.3f\n", hiDuck, hiFree);
        CHECK (hiDuck < hiFree * 0.5f, "SJÄLV: kicken i signalen duckar resten");
    }

    // 5. SUBDIST: övertoner på en 50 Hz-sinus, ingen likström
    {
        Params p; p.duckDepth = 0.f; p.drive = 0.8f; p.driveTone = 900.f;
        auto in = sines (FS, { { 50.f, 0.5f } });
        auto clean = run (Params { [] { Params q; q.duckDepth = 0.f; return q; }() }, in);
        auto out = run (p, in);
        const float h2 = mag (out.l, FS / 2, FS, 100.f), h3 = mag (out.l, FS / 2, FS, 150.f);
        const float c2 = mag (clean.l, FS / 2, FS, 100.f);
        double dc = 0; for (size_t i = FS / 2; i < FS; ++i) dc += out.l[i]; dc /= FS / 2;
        std::printf ("      2:a %.3f (ren %.4f), 3:e %.3f, DC %.4f\n", h2, c2, h3, dc);
        CHECK (h2 > 0.02f && h2 > c2 * 10.f, "SUBDIST: jämna övertoner");
        CHECK (h3 > 0.02f, "SUBDIST: udda övertoner");
        CHECK (std::abs (dc) < 0.01, "SUBDIST: ingen likström");
        if (wav) writeWav ("kold_subdist.wav", out);
    }

    // 6. TRANSIENT: ANSLAG +1 ger skarpare slag mot svansen, SVANS +1 längre svans
    {
        const size_t len = beat * 2;
        Stereo in { std::vector<float> (len), std::vector<float> (len) };
        addKick (in, 0, 0.5f, 0.4f);
        Params n; n.duckDepth = 0.f;
        Params a = n; a.attack = 1.f;
        Params s = n; s.sustain = 1.f;
        auto on = run (n, in), oa = run (a, in), os = run (s, in);
        const float headN = peak (on.l, 0, 1200), headA = peak (oa.l, 0, 1200);
        const float tailN = rms (on.l, 6000, 14000), tailS = rms (os.l, 6000, 14000);
        std::printf ("      anslag %.3f -> %.3f, svans %.3f -> %.3f\n", headN, headA, tailN, tailS);
        CHECK (headA > headN * 1.2f, "ANSLAG: slaget blir hårdare");
        CHECK (tailS > tailN * 1.2f, "SVANS: svansen blir längre");
    }

    // 7. Stresstest: slumpade rattar, brus, kickar, alla lägen. Inget NaN, aldrig över 0 dBFS.
    {
        std::mt19937 rng (7);
        std::uniform_real_distribution<float> u (0.f, 1.f);
        bool ok = true; float pk = 0;
        for (int round = 0; round < 40; ++round)
        {
            Params p;
            p.source = (Source) (round % 3);
            for (auto& c : p.curve) c = u (rng);
            p.duckDepth = u (rng); p.duckBeats = 0.25f + u (rng) * 1.75f; p.duckOnlyLow = u (rng) < 0.5f;
            p.threshold = u (rng); p.kickMs = 40.f + u (rng) * 360.f;
            p.attack = u (rng) * 2 - 1; p.sustain = u (rng) * 2 - 1;
            p.subHz = 30.f + u (rng) * 60.f; p.subLevel = u (rng); p.subDecay = 0.05f + u (rng) * 1.45f;
            p.mode = (GuillotineMode) (round & 1);
            p.crossoverHz = 60.f + u (rng) * 190.f; p.drive = u (rng); p.driveTone = 150.f + u (rng) * 1850.f;
            p.monoLow = u (rng) < 0.5f; p.mix = u (rng); p.outputDb = -12.f + u (rng) * 18.f;
            const size_t len = FS;
            Stereo in { std::vector<float> (len), std::vector<float> (len) };
            for (size_t i = 0; i < len; ++i) { in.l[i] = (u (rng) * 2 - 1) * 0.6f; in.r[i] = (u (rng) * 2 - 1) * 0.6f; }
            for (int k = 0; k < 4; ++k) addKick (in, (size_t) k * beat, 1.2f);
            Stereo sc = in;
            auto out = run (p, in, &sc, 64 + round * 13, 70.0 + round * 4.0);
            ok = ok && finite (out);
            pk = std::max ({ pk, peak (out.l, 0, len), peak (out.r, 0, len) });
        }
        std::printf ("      högsta topp %.3f\n", pk);
        CHECK (ok, "stress: inga NaN/Inf");
        CHECK (pk <= 1.0f, "stress: aldrig över 0 dBFS");
    }

    std::printf (failures ? "\nRESULTAT: %d FEL\n" : "\nRESULTAT: allt OK\n", failures);
    return failures ? 1 : 0;
}
