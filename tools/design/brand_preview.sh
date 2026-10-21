#!/bin/sh
# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 The SMY-OS Authors
#
# Render the logo concept preview sheet for review:
#   build/brand/concepts-preview.png
# One row per concept: dark lockup, light lockup, monochrome lockup, and the
# symbol rendered at 16, 32 and 64 px (enlarged without smoothing, so the
# real pixels are visible). Language: POSIX sh (ADR 0002).
# Tools: rsvg-convert (librsvg), montage/convert (ImageMagick) — build tools
# only, recorded in LICENSES.md.

set -eu

root=$(cd "$(dirname "$0")/../.." && pwd)
src="$root/design/brand/concepts"
out="$root/build/brand"
tmp="$out/preview-tmp"
rm -rf "$tmp"
mkdir -p "$tmp"

rows=""
for c in a b c; do
    rsvg-convert -w 700 "$src/concept-$c-lockup-dark.svg" -o "$tmp/$c-1.png"
    rsvg-convert -w 700 "$src/concept-$c-lockup-light.svg" -o "$tmp/$c-2.png"
    rsvg-convert -w 700 -b white "$src/concept-$c-mono.svg" -o "$tmp/$c-3.png"
    for z in 16 32 64; do
        rsvg-convert -w "$z" -b '#080B12' "$src/concept-$c-symbol.svg" -o "$tmp/$c-px$z.png"
        convert "$tmp/$c-px$z.png" -filter point -resize 128x128 "$tmp/$c-px$z-big.png"
    done
    montage "$tmp/$c-px16-big.png" "$tmp/$c-px32-big.png" "$tmp/$c-px64-big.png" \
        -tile 3x1 -geometry +6+6 -background '#9AA3B5' "$tmp/$c-4.png"
    name=$(sed -n 's/.*<title>SMY-OS logo concept \(.\): \([A-Za-z]*\).*/\1 — \2/p' "$src/concept-$c-symbol.svg")
    montage -label "$name" "$tmp/$c-1.png" "$tmp/$c-2.png" "$tmp/$c-3.png" "$tmp/$c-4.png" \
        -tile 4x1 -geometry 700x200+10+10 -pointsize 22 -background '#C9CFDA' "$tmp/row-$c.png"
    rows="$rows $tmp/row-$c.png"
done
# shellcheck disable=SC2086
convert $rows -append "$out/concepts-preview.png"
rm -rf "$tmp"
echo "brand_preview: $out/concepts-preview.png"
