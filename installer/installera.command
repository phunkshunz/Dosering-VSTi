#!/bin/bash
# KOLDBARAREN - MASKINHALLEN / Phunkshunz AUDIO
# Installerar VST3 och AU i din anvandarmapp. Inget losenord behovs.

cd "$(dirname "$0")" || exit 1

VST3_DIR="$HOME/Library/Audio/Plug-Ins/VST3"
AU_DIR="$HOME/Library/Audio/Plug-Ins/Components"
mkdir -p "$VST3_DIR" "$AU_DIR"

echo ""
echo "  KOLDBARAREN - installerar..."
echo ""

rm -rf "$VST3_DIR/KOLDBARAREN.vst3" "$AU_DIR/KOLDBARAREN.component"
cp -R "KOLDBARAREN.vst3" "$VST3_DIR/"      && echo "  OK  VST3 -> $VST3_DIR"
cp -R "KOLDBARAREN.component" "$AU_DIR/"   && echo "  OK  AU   -> $AU_DIR"

# Ta bort karantan sa att macOS inte blockerar pluginen
xattr -dr com.apple.quarantine "$VST3_DIR/KOLDBARAREN.vst3" 2>/dev/null
xattr -dr com.apple.quarantine "$AU_DIR/KOLDBARAREN.component" 2>/dev/null

# Lat macOS hitta den nya AU-pluginen direkt
killall -9 AudioComponentRegistrar 2>/dev/null

echo ""
echo "  Klart! Starta om Bitwig och skanna plugins."
echo ""
read -r -p "  Tryck Enter for att stanga..."
