#pragma once

// DOSERING: svensk text -> engelsk text. Nyckeln skrivs exakt som i koden.
// Presetnamnen står kvar på svenska, de är en del av maskinen.

namespace sprak
{
inline constexpr Par kOrdlista[] =
{
    { "DUCKAR", "DUCKS" },
    { "FYLLER", "FILLS" },
    { "RIVER", "TEARS" },
    { "LYSSNA", "LISTEN" },
    { "UT", "OUT" },
    { "PUMPA ALLT", "PUMP ALL" },
    { "SLUMPA", "DICE" },
    { "Hur stor dos botten får. Först duckar ljudet i takt, sedan fylls luckan med sub, sist rivs botten. Dubbelklicka för 50.", "How big a dose the low end gets. First the sound ducks in time, then the gap is filled with sub, last the low end is torn up. Double-click for 50." },
    { "Pumpar på varje fjärdedel, i takt med DAW:en.", "Pumps on every quarter note, in time with the DAW." },
    { "Pumpar på varje åttondel.", "Pumps on every eighth note." },
    { "Pumpar mitt emellan slagen.", "Pumps between the beats." },
    { "Lyssnar efter kicken i ljudet och pumpar på den. Behöver ingen takt.", "Listens for the kick in the sound and pumps on it. Needs no tempo." },
    { "Blanda in det orörda ljudet. Lågt värde ger parallellt tryck.", "Blend in the untouched sound. Low values give parallel squeeze." },
    { "Nivå ut.", "Output level." },
    { "På: hela ljudet duckar. Av: bara botten duckar och toppen står kvar.", "On: the whole sound ducks. Off: only the low end ducks and the top stays." },
    { "Ny karaktär i samma läge. Ratten står kvar.", "New character in the same mode. The knob stays put." },
    { "Föregående preset", "Previous preset" },
    { "Nästa preset", "Next preset" },
    { "Välj preset", "Choose preset" },
};
} // namespace sprak
