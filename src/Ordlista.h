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
    { "LÄNGD", "LENGTH" },
    { "FORM", "SHAPE" },
    { "ANSLAG", "ATTACK" },
    { "SUBTON", "SUB PITCH" },
    { "FALL", "DECAY" },
    { "RIV", "TEAR" },
    { "Hur djupt ljudet duckar när den stora ratten är uppe.", "How deep the sound ducks when the big knob is up." },
    { "Hur stor del av takten pumpen tar innan ljudet är tillbaka.", "How much of the beat the pump takes before the sound is back." },
    { "Pumpens böj. Lågt kommer tillbaka direkt, högt ligger kvar nere och kommer snabbt på slutet.", "The bend of the pump. Low comes back at once, high stays down and returns quickly at the end." },
    { "Vänster mjukar upp slaget i ljudet, höger gör det hårdare.", "Left softens the attack of the sound, right makes it harder." },
    { "Hur mycket sub som fylls på i luckan efter kicken.", "How much sub is filled into the gap after the kick." },
    { "Subens tonhöjd. Stäm den mot låtens grundton.", "The pitch of the sub. Tune it to the root note of the track." },
    { "Hur länge suben klingar innan den dör ut.", "How long the sub rings before it dies out." },
    { "Hur mycket botten rivs upp av mättning högst upp på den stora ratten.", "How much the low end is torn up by saturation at the top of the big knob." },
};
} // namespace sprak
