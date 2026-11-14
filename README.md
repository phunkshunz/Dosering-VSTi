# DOSERING

MASKINHALLEN · ONE KNOB. En ratt doserar botten: ljudet duckar i takt, luckan fylls med en sub som startar i fas när kicken släpper, och sist rivs botten upp. Ingen sidechain-koppling behövs.

- **Ratten (MÄNGD):** 0 = orört. Först DUCKAR, från ca 20 FYLLER (sub), från ca 45 RIVER (mättning). Lamporna visar vad som är igång.
- **Lägen:** 1/4, 1/8 och OFFBEAT följer DAW:ens tempo. LYSSNA följer kicken i ljudet.
- **MIX / UT / PUMPA ALLT / SLUMPA** under lägena.
- **60 presets**, 15 per läge.
- SV | EN-knapp, gemensam för alla MASKINHALLEN-maskiner.

Ljudmotorn kommer från KÖLDBÄRAREN 0.1 som låg i det här repot (`src/KoldEngine.h`). `src/Dose.h` översätter ratten och presetet till motorns inställningar.

## Installera

- **Mac:** packa upp zippen, öppna Terminal och kör `bash installera.command` i mappen.
- **Windows:** kopiera `DOSERING.vst3` till `C:\Program Files\Common Files\VST3`.
- **Linux:** kör `installera-linux.sh`.

Starta om din DAW. DOSERING finns bland effekterna.

## Bygga

GitHub Actions bygger Mac (VST3 + AU), Windows (VST3) och Linux (VST3 + LV2) vid varje push till `main`.

Motortestet körs utan JUCE:

    g++ -std=c++17 -O2 -Isrc tests/engine_test.cpp -o engine_test && ./engine_test
