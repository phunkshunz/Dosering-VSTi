#pragma once

// DOSERING · MASKINHALLEN ONE KNOB. Ratten doserar botten: först duckar ljudet i takt, sedan fylls
// luckorna med en sub som startar i fas när kicken släpper, sist rivs botten upp av mättning.
// Här översätts ratten + karaktären (presetet) till motorns inställningar. Ren C++ utan JUCE.

#include "KoldEngine.h"

namespace dos
{
enum Mode { Fjardedel = 0, Attondel = 1, Offbeat = 2, Lyssna = 3 };

// Det som ratten och presetet bestämmer.
struct Settings
{
    float amount = 0.0f;     // MÄNGD: den stora ratten, 0..1
    int   mode   = Fjardedel;
    float duck   = 0.85f;    // hur djupt ratten duckar, 0..1
    float sub    = 0.5f;     // hur mycket sub som fylls på, 0..1
    float dist   = 0.3f;     // hur mycket botten rivs, 0..1
    float anslag = 0.0f;     // -1..1: slaget i ljudet
    float svans  = 0.0f;     // -1..1: det som kommer efter slaget
    float langd  = 0.6f;     // kurvans längd, andel av takten (0,25..1)
    float form   = 1.8f;     // kurvans böj: 1 = rak, högre = stannar nere längre och kommer snabbt
    float hall   = 0.05f;    // hur länge kurvan ligger kvar i botten, 0..0,5
    float subHz  = 45.0f;
    float subFall = 0.4f;    // sekunder
    float brytHz = 120.0f;   // under den här frekvensen är det botten
    float tonHz  = 600.0f;   // tonfilter efter mättningen
    bool  allt   = false;    // PUMPA ALLT: ducka hela ljudet, inte bara botten
    bool  spik   = false;    // suben kapas tvärt i stället för att dö ut
    float mix    = 1.0f;
    float outDb  = 0.0f;
};

inline kold::Params toParams (const Settings& s)
{
    using kold::clamp01;
    kold::Params p;
    const float a = clamp01 (s.amount);
    const float grid = s.mode == Attondel ? 0.5f : 1.0f;

    p.source = s.mode == Lyssna ? kold::Source::Self : kold::Source::Takt;
    p.gridBeats = grid;
    p.gridOffset = s.mode == Offbeat ? 0.5f : 0.0f;
    p.kickMs = s.mode == Attondel ? 80.0f : 140.0f;

    // rattens tre steg: duck först, sedan suben, sist rivet
    p.duckDepth = clamp01 (s.duck) * clamp01 (a / 0.6f);
    p.subLevel  = clamp01 (s.sub)  * clamp01 ((a - 0.2f) / 0.6f);
    p.drive     = clamp01 (s.dist) * clamp01 ((a - 0.45f) / 0.55f);
    p.attack    = std::max (-1.0f, std::min (1.0f, s.anslag)) * a;
    p.sustain   = std::max (-1.0f, std::min (1.0f, s.svans)) * a;

    p.duckBeats = std::max (0.1f, std::min (1.0f, s.langd)) * grid;
    p.duckOnlyLow = ! s.allt;
    const float h = std::max (0.0f, std::min (0.5f, s.hall));
    for (int i = 0; i < kold::kCurvePoints; ++i)
    {
        const float t = (float) i / (float) (kold::kCurvePoints - 1);
        const float x = t <= h ? 0.0f : std::min (1.0f, (t - h) / (0.95f - h));
        p.curve[(size_t) i] = clamp01 (std::pow (x, std::max (0.3f, s.form)));
    }

    p.subHz = std::max (30.0f, std::min (90.0f, s.subHz));
    p.subDecay = std::max (0.05f, std::min (1.5f, s.subFall));
    p.mode = s.spik ? kold::GuillotineMode::Spikrakt : kold::GuillotineMode::Slap;
    p.crossoverHz = std::max (60.0f, std::min (250.0f, s.brytHz));
    p.driveTone = std::max (150.0f, std::min (2000.0f, s.tonHz));
    p.monoLow = true;

    // med ratten på noll går ljudet rakt igenom, orört
    p.mix = clamp01 (s.mix) * clamp01 (a / 0.04f);
    p.outputDb = s.outDb;
    return p;
}

struct Preset
{
    const char* name;
    int mode;
    float amount, duck, sub, dist, anslag, svans, langd, form, hall, subHz, subFall, brytHz, tonHz;
    bool allt, spik;
    float mix;
};

// 60 presets, 15 per läge: 1/4, 1/8, OFFBEAT och LYSSNA (följer kicken i ljudet)
inline constexpr Preset kPresets[] =
{
    { "KYLRUM", 0, 0.50f, 0.85f, 0.50f, 0.30f, 0.20f, 0.00f, 0.60f, 1.8f, 0.05f, 45.0f, 0.40f, 120.0f, 600.0f, false, false, 1.00f },
    { "TUNG PUMP", 0, 0.60f, 1.00f, 0.30f, 0.20f, 0.00f, 0.20f, 0.80f, 2.2f, 0.10f, 42.0f, 0.50f, 110.0f, 500.0f, true, false, 1.00f },
    { "LÄTT ANDNING", 0, 0.35f, 0.55f, 0.00f, 0.00f, 0.00f, 0.10f, 0.50f, 1.4f, 0.00f, 45.0f, 0.40f, 120.0f, 600.0f, true, false, 1.00f },
    { "BETONGBOTTEN", 0, 0.70f, 0.90f, 0.80f, 0.50f, 0.30f, 0.00f, 0.55f, 2.0f, 0.08f, 40.0f, 0.60f, 100.0f, 450.0f, false, false, 1.00f },
    { "KOMPRESSORRUM", 0, 0.65f, 0.95f, 0.40f, 0.60f, 0.40f, -0.20f, 0.70f, 2.6f, 0.12f, 48.0f, 0.35f, 130.0f, 800.0f, true, false, 1.00f },
    { "SUGPUMP", 0, 0.75f, 1.00f, 0.20f, 0.10f, 0.00f, 0.50f, 0.95f, 3.0f, 0.15f, 45.0f, 0.40f, 120.0f, 600.0f, true, false, 1.00f },
    { "BARA SUBBEN", 0, 0.55f, 0.30f, 1.00f, 0.20f, 0.00f, 0.00f, 0.40f, 1.5f, 0.00f, 38.0f, 0.80f, 90.0f, 400.0f, false, false, 1.00f },
    { "GILJOTIN", 0, 0.80f, 0.70f, 0.90f, 0.40f, 0.20f, -0.30f, 0.35f, 1.2f, 0.00f, 50.0f, 0.25f, 110.0f, 700.0f, false, true, 1.00f },
    { "DJUP KÄLLARE", 0, 0.62f, 0.80f, 0.70f, 0.30f, 0.00f, 0.20f, 0.65f, 2.0f, 0.10f, 34.0f, 1.00f, 95.0f, 350.0f, false, false, 1.00f },
    { "LAGERHALL", 0, 0.45f, 0.70f, 0.30f, 0.00f, 0.10f, 0.30f, 0.75f, 1.6f, 0.05f, 44.0f, 0.50f, 140.0f, 600.0f, true, false, 1.00f },
    { "HÅRD SIDECHAIN", 0, 0.85f, 1.00f, 0.00f, 0.30f, 0.50f, -0.40f, 0.45f, 1.0f, 0.18f, 45.0f, 0.40f, 120.0f, 900.0f, true, false, 1.00f },
    { "OLJETRYCK", 0, 0.58f, 0.75f, 0.60f, 0.80f, 0.20f, 0.00f, 0.60f, 1.8f, 0.06f, 46.0f, 0.45f, 150.0f, 1100.0f, false, false, 1.00f },
    { "PARALLELL PUMP", 0, 0.90f, 1.00f, 0.50f, 0.40f, 0.00f, 0.30f, 0.70f, 2.4f, 0.10f, 43.0f, 0.50f, 120.0f, 600.0f, true, false, 0.50f },
    { "FROSTSKADA", 0, 0.72f, 0.60f, 0.60f, 1.00f, 0.40f, -0.20f, 0.50f, 1.6f, 0.04f, 52.0f, 0.30f, 170.0f, 1600.0f, false, true, 1.00f },
    { "STILLA DRIFT", 0, 0.30f, 0.45f, 0.25f, 0.10f, 0.00f, 0.00f, 0.55f, 1.5f, 0.00f, 45.0f, 0.40f, 120.0f, 600.0f, false, false, 1.00f },
    { "ÅTTONDELSPUMP", 1, 0.55f, 0.85f, 0.20f, 0.10f, 0.00f, 0.10f, 0.90f, 1.6f, 0.05f, 45.0f, 0.20f, 120.0f, 600.0f, true, false, 1.00f },
    { "MASKINGEVÄR", 1, 0.70f, 1.00f, 0.00f, 0.40f, 0.50f, -0.50f, 0.60f, 1.0f, 0.10f, 45.0f, 0.20f, 120.0f, 1000.0f, true, false, 1.00f },
    { "TRUMMANDE RÖR", 1, 0.60f, 0.70f, 0.70f, 0.50f, 0.20f, 0.00f, 0.70f, 1.4f, 0.00f, 55.0f, 0.15f, 130.0f, 800.0f, false, true, 1.00f },
    { "SNABB KOLV", 1, 0.50f, 0.80f, 0.40f, 0.20f, 0.30f, -0.20f, 0.80f, 2.0f, 0.08f, 48.0f, 0.18f, 120.0f, 700.0f, false, false, 1.00f },
    { "VIBRATOR", 1, 0.80f, 0.95f, 0.50f, 0.70f, 0.00f, 0.00f, 0.95f, 2.5f, 0.15f, 60.0f, 0.12f, 140.0f, 1200.0f, true, true, 1.00f },
    { "GALOPP", 1, 0.45f, 0.60f, 0.30f, 0.00f, 0.20f, 0.20f, 0.50f, 1.2f, 0.00f, 45.0f, 0.20f, 120.0f, 600.0f, true, false, 1.00f },
    { "TREMOLOBOTTEN", 1, 0.65f, 0.90f, 0.60f, 0.30f, 0.00f, 0.00f, 0.99f, 1.0f, 0.00f, 42.0f, 0.22f, 100.0f, 500.0f, false, false, 1.00f },
    { "RULLBAND", 1, 0.58f, 0.75f, 0.20f, 0.60f, 0.10f, 0.30f, 0.85f, 1.8f, 0.05f, 45.0f, 0.20f, 160.0f, 900.0f, true, false, 1.00f },
    { "SYMASKIN", 1, 0.88f, 1.00f, 0.30f, 0.20f, 0.60f, -0.60f, 0.40f, 1.0f, 0.20f, 45.0f, 0.15f, 120.0f, 1400.0f, true, false, 1.00f },
    { "PULS 16", 1, 0.52f, 0.65f, 0.80f, 0.10f, 0.00f, 0.00f, 0.60f, 1.5f, 0.00f, 36.0f, 0.25f, 90.0f, 400.0f, false, true, 1.00f },
    { "STRESSAD PUMP", 1, 0.75f, 0.85f, 0.40f, 0.90f, 0.30f, -0.10f, 0.75f, 1.3f, 0.06f, 50.0f, 0.18f, 150.0f, 1500.0f, true, false, 1.00f },
    { "PARALLELL 1/8", 1, 0.95f, 1.00f, 0.40f, 0.50f, 0.00f, 0.20f, 0.90f, 2.0f, 0.10f, 45.0f, 0.20f, 120.0f, 700.0f, true, false, 0.45f },
    { "LÄTTA FÖTTER", 1, 0.32f, 0.50f, 0.00f, 0.00f, 0.10f, 0.10f, 0.70f, 1.4f, 0.00f, 45.0f, 0.20f, 120.0f, 600.0f, true, false, 1.00f },
    { "KUGGHJUL", 1, 0.68f, 0.80f, 0.50f, 0.40f, 0.40f, -0.30f, 0.55f, 1.1f, 0.12f, 47.0f, 0.16f, 125.0f, 1000.0f, false, false, 1.00f },
    { "DUBBELSLAG", 1, 0.60f, 0.70f, 0.90f, 0.30f, 0.00f, 0.00f, 0.45f, 1.6f, 0.00f, 40.0f, 0.30f, 105.0f, 450.0f, false, false, 1.00f },
    { "OFFBEATLYFT", 2, 0.55f, 0.80f, 0.00f, 0.10f, 0.00f, 0.20f, 0.60f, 1.6f, 0.05f, 45.0f, 0.40f, 120.0f, 600.0f, true, false, 1.00f },
    { "BAKSLAG", 2, 0.65f, 0.95f, 0.00f, 0.30f, 0.30f, -0.20f, 0.50f, 1.2f, 0.10f, 45.0f, 0.40f, 120.0f, 900.0f, true, false, 1.00f },
    { "MOTVIKT", 2, 0.50f, 0.70f, 0.40f, 0.20f, 0.00f, 0.10f, 0.70f, 2.0f, 0.05f, 48.0f, 0.30f, 120.0f, 600.0f, false, false, 1.00f },
    { "GUNGBRÄDA", 2, 0.72f, 1.00f, 0.00f, 0.00f, 0.00f, 0.40f, 0.95f, 2.6f, 0.10f, 45.0f, 0.40f, 120.0f, 600.0f, true, false, 1.00f },
    { "BAKVÄND PUMP", 2, 0.60f, 0.85f, 0.30f, 0.50f, 0.20f, 0.00f, 0.80f, 3.0f, 0.20f, 44.0f, 0.35f, 140.0f, 800.0f, true, false, 1.00f },
    { "HATTLUCKA", 2, 0.45f, 0.60f, 0.00f, 0.00f, 0.40f, -0.30f, 0.35f, 1.0f, 0.05f, 45.0f, 0.40f, 200.0f, 1500.0f, true, false, 1.00f },
    { "SYNKOP", 2, 0.68f, 0.90f, 0.50f, 0.40f, 0.10f, 0.00f, 0.55f, 1.4f, 0.00f, 52.0f, 0.25f, 130.0f, 900.0f, false, true, 1.00f },
    { "EFTERSLAG", 2, 0.40f, 0.55f, 0.20f, 0.10f, 0.00f, 0.20f, 0.65f, 1.8f, 0.00f, 45.0f, 0.40f, 120.0f, 600.0f, true, false, 1.00f },
    { "MOTTRYCK", 2, 0.82f, 1.00f, 0.20f, 0.70f, 0.30f, -0.20f, 0.60f, 1.5f, 0.12f, 46.0f, 0.30f, 150.0f, 1300.0f, true, false, 1.00f },
    { "SKEV VÅG", 2, 0.58f, 0.75f, 0.60f, 0.30f, 0.00f, 0.30f, 0.90f, 2.2f, 0.08f, 38.0f, 0.50f, 100.0f, 450.0f, false, false, 1.00f },
    { "PARALLELL OFFBEAT", 2, 0.92f, 1.00f, 0.30f, 0.40f, 0.00f, 0.20f, 0.75f, 2.0f, 0.10f, 45.0f, 0.40f, 120.0f, 700.0f, true, false, 0.50f },
    { "SLÄPANDE KEDJA", 2, 0.63f, 0.80f, 0.40f, 0.60f, 0.20f, 0.20f, 0.85f, 2.8f, 0.18f, 45.0f, 0.45f, 135.0f, 750.0f, true, false, 1.00f },
    { "LUFTFICKA", 2, 0.36f, 0.50f, 0.00f, 0.00f, 0.00f, 0.00f, 0.50f, 1.3f, 0.00f, 45.0f, 0.40f, 120.0f, 600.0f, true, false, 1.00f },
    { "PENDEL", 2, 0.70f, 0.90f, 0.70f, 0.20f, 0.00f, 0.00f, 0.99f, 1.0f, 0.00f, 41.0f, 0.40f, 95.0f, 400.0f, false, false, 1.00f },
    { "NATTSKIFT", 2, 0.54f, 0.70f, 0.50f, 0.50f, 0.30f, -0.10f, 0.60f, 1.7f, 0.06f, 49.0f, 0.30f, 140.0f, 1000.0f, false, false, 1.00f },
    { "FÖLJ KICKEN", 3, 0.55f, 0.80f, 0.50f, 0.20f, 0.20f, 0.00f, 0.50f, 1.6f, 0.05f, 45.0f, 0.40f, 120.0f, 600.0f, false, false, 1.00f },
    { "SUB UNDER KICKEN", 3, 0.65f, 0.40f, 1.00f, 0.30f, 0.00f, 0.00f, 0.40f, 1.5f, 0.00f, 40.0f, 0.60f, 95.0f, 400.0f, false, false, 1.00f },
    { "KICKEN ÄTER BASEN", 3, 0.75f, 1.00f, 0.20f, 0.10f, 0.00f, 0.20f, 0.60f, 2.0f, 0.10f, 45.0f, 0.40f, 130.0f, 600.0f, false, false, 1.00f },
    { "SLAGFÖRSTÄRKARE", 3, 0.60f, 0.30f, 0.30f, 0.40f, 1.00f, -0.50f, 0.30f, 1.2f, 0.00f, 48.0f, 0.30f, 120.0f, 900.0f, false, false, 1.00f },
    { "LÅNG SVANS", 3, 0.58f, 0.50f, 0.70f, 0.20f, -0.20f, 1.00f, 0.70f, 1.8f, 0.05f, 43.0f, 0.90f, 110.0f, 500.0f, false, false, 1.00f },
    { "RIVEN BOTTEN", 3, 0.80f, 0.60f, 0.50f, 1.00f, 0.30f, 0.00f, 0.50f, 1.5f, 0.05f, 50.0f, 0.35f, 180.0f, 1800.0f, false, false, 1.00f },
    { "TORR KICK", 3, 0.50f, 0.70f, 0.00f, 0.10f, 0.60f, -0.80f, 0.35f, 1.0f, 0.10f, 45.0f, 0.40f, 120.0f, 800.0f, false, false, 1.00f },
    { "DOVT MULLER", 3, 0.68f, 0.50f, 0.90f, 0.60f, -0.30f, 0.60f, 0.60f, 1.6f, 0.00f, 36.0f, 1.20f, 90.0f, 300.0f, false, false, 1.00f },
    { "KIRURGEN", 3, 0.72f, 0.85f, 0.80f, 0.20f, 0.20f, -0.30f, 0.30f, 1.0f, 0.00f, 55.0f, 0.20f, 115.0f, 700.0f, false, true, 1.00f },
    { "HELA MIXEN PUMPAR", 3, 0.62f, 0.90f, 0.00f, 0.00f, 0.00f, 0.30f, 0.70f, 2.2f, 0.10f, 45.0f, 0.40f, 120.0f, 600.0f, true, false, 1.00f },
    { "PARALLELL BOTTEN", 3, 0.90f, 0.90f, 0.80f, 0.70f, 0.40f, 0.00f, 0.55f, 1.6f, 0.05f, 44.0f, 0.45f, 130.0f, 900.0f, false, false, 0.50f },
    { "TJOCK OCH LÅNGSAM", 3, 0.57f, 0.65f, 0.60f, 0.40f, -0.40f, 0.80f, 0.85f, 2.6f, 0.12f, 39.0f, 0.80f, 100.0f, 400.0f, false, false, 1.00f },
    { "SNABB OCH VASS", 3, 0.66f, 0.75f, 0.30f, 0.50f, 0.90f, -0.60f, 0.30f, 1.0f, 0.05f, 54.0f, 0.18f, 140.0f, 1400.0f, false, true, 1.00f },
    { "MASKINRUMMET", 3, 0.78f, 0.85f, 0.70f, 0.80f, 0.30f, 0.10f, 0.55f, 1.8f, 0.08f, 46.0f, 0.40f, 150.0f, 1100.0f, false, false, 1.00f },
    { "SISTA DOSEN", 3, 1.00f, 1.00f, 1.00f, 1.00f, 0.50f, 0.00f, 0.60f, 2.0f, 0.10f, 42.0f, 0.50f, 120.0f, 800.0f, false, false, 1.00f },
};

inline constexpr int kNumPresets = (int) (sizeof (kPresets) / sizeof (kPresets[0]));

inline Settings toSettings (const Preset& p)
{
    Settings s;
    s.amount = p.amount; s.mode = p.mode; s.duck = p.duck; s.sub = p.sub; s.dist = p.dist;
    s.anslag = p.anslag; s.svans = p.svans; s.langd = p.langd; s.form = p.form; s.hall = p.hall;
    s.subHz = p.subHz; s.subFall = p.subFall; s.brytHz = p.brytHz; s.tonHz = p.tonHz;
    s.allt = p.allt; s.spik = p.spik; s.mix = p.mix;
    return s;
}
} // namespace dos
