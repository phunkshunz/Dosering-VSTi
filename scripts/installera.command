#!/bin/bash
# Installerar DOSERING. Kör från Terminal:  bash installera.command
set -e
cd "$(dirname "$0")"

VST3="$HOME/Library/Audio/Plug-Ins/VST3"
AU="$HOME/Library/Audio/Plug-Ins/Components"
mkdir -p "$VST3" "$AU"

rm -rf "$VST3/DOSERING.vst3" "$AU/DOSERING.component"
cp -R DOSERING.vst3 "$VST3/"
cp -R DOSERING.component "$AU/"

# Pluginet är osignerat, så macOS spärrar det. Detta tar bort spärren.
xattr -dr com.apple.quarantine "$VST3/DOSERING.vst3" "$AU/DOSERING.component" 2>/dev/null || true

echo ""
echo "Klart! DOSERING ligger nu i:"
echo "  $VST3"
echo "Stäng Bitwig helt och starta igen. DOSERING finns bland effekterna."
echo "Lägg DOSERING på basen eller hela mixen och vrid på ratten."
