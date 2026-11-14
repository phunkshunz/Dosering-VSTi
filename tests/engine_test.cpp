// DOSERING · motortest. Körs utan JUCE:  g++ -std=c++17 -O2 -Isrc tests/engine_test.cpp -o engine_test && ./engine_test
#include "Dose.h"
#include <cstdio>
#include <vector>

static int fails = 0;
#define CHECK(x) do { if (! (x)) { std::printf ("FEL: %s (rad %d)\n", #x, __LINE__); ++fails; } } while (0)

static const double kBpm = 130.0;

// kick på varje fjärdedel + en liggande bas på 55 Hz + lite hatt
static void loop (std::vector<float>& L, std::vector<float>& R, double sr, float level)
{
    const double beat = sr * 60.0 / kBpm;
    unsigned rng = 99;
    for (size_t i = 0; i < L.size(); ++i)
    {
        const double t = std::fmod ((double) i, beat) / sr;
        float x = (float) (std::sin (2.0 * 3.14159265 * (45.0 + 160.0 * std::exp (-t * 35.0)) * t) * std::exp (-t * 9.0)) * 0.8f;
        x += (float) std::sin (2.0 * 3.14159265 * 55.0 * (double) i / sr) * 0.35f;
        const double th = std::fmod ((double) i + beat * 0.5, beat) / sr;
        rng = rng * 1664525u + 1013904223u;
        x += ((float) ((rng >> 8) & 0xffff) / 32768.0f - 1.0f) * (float) std::exp (-th * 60.0) * 0.2f;
        L[i] = x * level; R[i] = x * level;
    }
}

static void run (kold::Engine& e, const dos::Settings& s, std::vector<float>& L, std::vector<float>& R, double sr)
{
    e.setParams (dos::toParams (s));
    const double beatsPerSample = kBpm / 60.0 / sr;
    for (size_t pos = 0; pos < L.size(); pos += 64)
    {
        const int n = (int) std::min<size_t> (64, L.size() - pos);
        e.setTransport (kBpm, (double) pos * beatsPerSample, true);
        e.process (L.data() + pos, R.data() + pos, nullptr, nullptr, n);
    }
}

static double rms (const std::vector<float>& v, size_t a, size_t b)
{
    double s = 0; for (size_t i = a; i < b; ++i) s += (double) v[i] * v[i];
    return std::sqrt (s / (double) (b - a));
}

int main()
{
    using namespace dos;
    for (double sr : { 44100.0, 48000.0, 96000.0 })
    {
        const size_t n = (size_t) (sr * 4.0);
        std::vector<float> inL (n), inR (n);
        loop (inL, inR, sr, 0.6f);
        const size_t beat = (size_t) (sr * 60.0 / kBpm);

        // 1) ratten på noll = orört ljud
        {
            kold::Engine e; e.prepare (sr);
            auto L = inL, R = inR;
            Settings s = toSettings (kPresets[0]); s.amount = 0.0f;
            run (e, s, L, R, sr);
            double diff = 0; for (size_t i = 0; i < n; ++i) diff = std::max (diff, (double) std::abs (L[i] - inL[i]));
            CHECK (diff < 1.0e-6);
        }

        // 2) varje preset: inga NaN, aldrig över 0 dB, och ljudet ändras tydligt
        for (int p = 0; p < kNumPresets; ++p)
        {
            kold::Engine e; e.prepare (sr);
            auto L = inL, R = inR;
            const Settings s = toSettings (kPresets[p]);
            run (e, s, L, R, sr);
            bool ok = true; float pk = 0;
            for (size_t i = 0; i < n; ++i) { if (! std::isfinite (L[i]) || ! std::isfinite (R[i])) ok = false; pk = std::max (pk, std::abs (L[i])); }
            CHECK (ok);
            CHECK (pk <= 1.0001f);
            double diff = 0; for (size_t i = n / 2; i < n; ++i) diff += std::abs (L[i] - inL[i]);
            CHECK (diff / (double) (n / 2) > 0.004);
            if (sr == 48000.0 && p % 6 == 0)
                std::printf ("%-20s topp %.2f  nivå %+5.1f dB  skillnad %.3f\n", kPresets[p].name, pk,
                             20.0 * std::log10 (rms (L, n / 2, n) / rms (inL, n / 2, n)), diff / (double) (n / 2));
        }

        // 3) pumpen: i 1/4-läget är ljudet svagare strax efter slaget än i slutet av takten
        {
            kold::Engine e; e.prepare (sr);
            auto L = inL, R = inR;
            Settings s; s.amount = 0.6f; s.mode = Fjardedel; s.duck = 1.0f; s.sub = 0.0f; s.dist = 0.0f; s.allt = true; s.langd = 0.6f;
            run (e, s, L, R, sr);
            const size_t b0 = beat * 6;
            const double early = rms (L, b0 + beat / 20, b0 + beat / 6) / rms (inL, b0 + beat / 20, b0 + beat / 6);
            const double late  = rms (L, b0 + beat * 8 / 10, b0 + beat * 95 / 100) / rms (inL, b0 + beat * 8 / 10, b0 + beat * 95 / 100);
            CHECK (early < 0.35);
            CHECK (late > 0.9);

            // OFFBEAT: samma sak men förskjutet en halv takt
            kold::Engine e2; e2.prepare (sr);
            auto L2 = inL, R2 = inR;
            s.mode = Offbeat;
            run (e2, s, L2, R2, sr);
            const size_t h0 = b0 + beat / 2;
            const double offEarly = rms (L2, h0 + beat / 20, h0 + beat / 6) / rms (inL, h0 + beat / 20, h0 + beat / 6);
            const double onBeat   = rms (L2, b0 + beat / 20, b0 + beat / 6) / rms (inL, b0 + beat / 20, b0 + beat / 6);
            CHECK (offEarly < 0.35);
            CHECK (onBeat > 0.8);
        }

        // 4) suben: fyller på energi i botten när den är på, och tystnar när ljudet in tystnar
        {
            kold::Engine e; e.prepare (sr);
            auto L = inL, R = inR;
            Settings s; s.amount = 1.0f; s.duck = 0.0f; s.sub = 1.0f; s.dist = 0.0f; s.subHz = 40.0f; s.subFall = 0.6f;
            run (e, s, L, R, sr);
            CHECK (rms (L, n / 2, n) > rms (inL, n / 2, n) * 1.05);

            kold::Engine e2; e2.prepare (sr);
            std::vector<float> zL (n, 0.0f), zR (n, 0.0f);
            run (e2, s, zL, zR, sr);
            float pk = 0; for (float v : zL) pk = std::max (pk, std::abs (v));
            CHECK (pk < 1.0e-4f);
        }
    }
    std::printf (fails == 0 ? "ALLT OK (%d presets)\n" : "%d FEL\n", fails == 0 ? dos::kNumPresets : fails);
    return fails == 0 ? 0 : 1;
}
