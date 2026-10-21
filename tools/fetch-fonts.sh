#!/bin/sh
# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 The SMY-OS Authors
#
# Download the pinned UI and monospace fonts (DESIGN_SYSTEM.md "Typography",
# LICENSES.md), verify the SHA-256 of each release archive, and extract the
# files SMY-OS uses into third_party/downloads/fonts/ (git-ignored).
# Language: POSIX sh (ADR 0002).
#
#   Inter 4.1             SIL OFL 1.1   https://github.com/rsms/inter
#   IBM Plex Mono 2.5.0   SIL OFL 1.1   https://github.com/IBM/plex
#                         (Reserved Font Name "Plex": a modified version,
#                         e.g. a subset, must not be called Plex)
#
# Upgrading a font is an explicit commit that changes version and SHA-256
# together.

set -eu

INTER_VERSION=4.1
INTER_SHA256=9883fdd4a49d4fb66bd8177ba6625ef9a64aa45899767dde3d36aa425756b11e
INTER_URL="https://github.com/rsms/inter/releases/download/v${INTER_VERSION}/Inter-${INTER_VERSION}.zip"

PLEX_MONO_VERSION=2.5.0
PLEX_MONO_SHA256=6d23f01257663d8cc49a0d64c22ced630b79e0e2a0ac08a0da86e9a38bbc481c
PLEX_MONO_URL="https://github.com/IBM/plex/releases/download/%40ibm%2Fplex-mono%40${PLEX_MONO_VERSION}/ibm-plex-mono.zip"

root=$(cd "$(dirname "$0")/.." && pwd)
cache="$root/third_party/downloads"
out="$cache/fonts"

# fetch <url> <sha256> <file>
fetch() {
    if [ -f "$3" ] && echo "$2  $3" | sha256sum -c --status; then
        echo "fetch-fonts: cached ${3#"$root"/} (SHA-256 OK)"
        return
    fi
    rm -f "$3" "$3.part"
    echo "fetch-fonts: downloading $1"
    curl --fail --silent --show-error --location --retry 3 --output "$3.part" "$1"
    if ! echo "$2  $3.part" | sha256sum -c --status; then
        echo "fetch-fonts: SHA-256 MISMATCH for $1" >&2
        echo "  expected: $2" >&2
        echo "  got:      $(sha256sum "$3.part" | cut -d' ' -f1)" >&2
        rm -f "$3.part"
        exit 1
    fi
    mv "$3.part" "$3"
}

mkdir -p "$cache"
fetch "$INTER_URL" "$INTER_SHA256" "$cache/Inter-${INTER_VERSION}.zip"
fetch "$PLEX_MONO_URL" "$PLEX_MONO_SHA256" "$cache/ibm-plex-mono-${PLEX_MONO_VERSION}.zip"

# Extract fresh every time, only the files in use (-j: no directories).
rm -rf "$out"
mkdir -p "$out/inter" "$out/plex-mono"
unzip -q -j -o "$cache/Inter-${INTER_VERSION}.zip" -d "$out/inter" \
    LICENSE.txt InterVariable.ttf InterVariable-Italic.ttf \
    extras/ttf/Inter-Regular.ttf extras/ttf/Inter-Medium.ttf \
    extras/ttf/Inter-SemiBold.ttf extras/ttf/Inter-Bold.ttf
unzip -q -j -o "$cache/ibm-plex-mono-${PLEX_MONO_VERSION}.zip" -d "$out/plex-mono" \
    ibm-plex-mono/LICENSE.txt \
    ibm-plex-mono/fonts/complete/ttf/IBMPlexMono-Regular.ttf \
    ibm-plex-mono/fonts/complete/ttf/IBMPlexMono-Medium.ttf \
    ibm-plex-mono/fonts/complete/ttf/IBMPlexMono-Bold.ttf \
    ibm-plex-mono/fonts/complete/ttf/IBMPlexMono-Italic.ttf
for lic in "$out/inter/LICENSE.txt" "$out/plex-mono/LICENSE.txt"; do
    if ! grep -q "SIL Open Font License, Version 1.1" "$lic"; then
        echo "fetch-fonts: $lic is not the SIL OFL 1.1 text" >&2
        exit 1
    fi
done
echo "fetch-fonts: Inter ${INTER_VERSION} and IBM Plex Mono ${PLEX_MONO_VERSION} ready in ${out#"$root"/}"
