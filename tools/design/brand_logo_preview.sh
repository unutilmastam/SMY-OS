#!/bin/sh
# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 The SMY-OS Authors
#
# Export the logo family (design/brand/logos/) to PNG and render a review
# sheet. Language: POSIX sh (ADR 0002).
#   build/brand/png/<name>-<size>.png   every SVG at the sizes below
#   build/brand/logo-family.png         one sheet: lockups on dark and light,
#                                       app icons, symbol and micro mark at
#                                       16/24/32/48 px (enlarged without
#                                       smoothing so real pixels show)
# Tools: rsvg-convert (librsvg), montage/convert (ImageMagick) — build tools
# only, recorded in LICENSES.md. PNGs are build outputs, never committed.

set -eu

root=$(cd "$(dirname "$0")/../.." && pwd)
src="$root/design/brand/logos"
out="$root/build/brand"
png="$out/png"
tmp="$out/family-tmp"
rm -rf "$png" "$tmp"
mkdir -p "$png" "$tmp"

# Square marks at 16-1024 px (docs/BRAND_PROMPT.md §6); wide lockups by
# width. The list matches tools/design/validate_brand.py.
for f in "$src"/*.svg; do
    name=$(basename "$f" .svg)
    case "$name" in
    smy-symbol-* | smy-micro-* | smy-app-icon* | smy-boot-logo | smy-developer-mark*)
        for z in 16 24 32 48 64 128 256 512 1024; do
            rsvg-convert -w "$z" -h "$z" "$f" -o "$png/$name-$z.png"
        done
        ;;
    *)
        for z in 480 960 1920; do
            rsvg-convert -w "$z" "$f" -o "$png/$name-$z.png"
        done
        ;;
    esac
done

cell() { # cell <svg-name> <bg> <out> : lockup centred on a 760x240 tile
    rsvg-convert -h 120 "$src/$1.svg" -o "$tmp/raw.png"
    convert -size 760x240 "xc:$2" "$tmp/raw.png" -gravity center -composite "$3"
}

cell smy-logo-horizontal-on-dark '#080B12' "$tmp/r1a.png"
cell smy-logo-horizontal-on-light '#F5F7FB' "$tmp/r1b.png"
cell smy-logo-horizontal-mono-white '#080B12' "$tmp/r2a.png"
cell smy-logo-horizontal-mono-black '#FFFFFF' "$tmp/r2b.png"
cell smy-installer-logo-on-dark '#080B12' "$tmp/r3a.png"
cell smy-installer-logo-on-light '#F5F7FB' "$tmp/r3b.png"
cell smy-developer-logo-on-dark '#080B12' "$tmp/r4a.png"
cell smy-wordmark-on-dark '#080B12' "$tmp/r4b.png"
cell smy-logo-stacked-on-dark '#080B12' "$tmp/r5a.png"
cell smy-logo-stacked-on-light '#F5F7FB' "$tmp/r5b.png"

# Icons row: app icons at 160 px, then small sizes enlarged with point filter.
icons=""
for n in smy-app-icon smy-app-icon-light smy-app-icon-small; do
    rsvg-convert -w 160 -h 160 "$src/$n.svg" -o "$tmp/$n.png"
    convert -size 200x240 xc:'#C9CFDA' "$tmp/$n.png" -gravity center -composite "$tmp/i-$n.png"
    icons="$icons $tmp/i-$n.png"
done
for n in smy-symbol-on-dark smy-micro-on-dark; do
    for z in 16 24 32 48; do
        rsvg-convert -w "$z" -h "$z" -b '#080B12' "$src/$n.svg" -o "$tmp/$n-$z.png"
        convert "$tmp/$n-$z.png" -filter point -resize 96x96 "$tmp/$n-$z-big.png"
    done
done
montage "$tmp"/smy-symbol-on-dark-*-big.png "$tmp"/smy-micro-on-dark-*-big.png \
    -tile 4x2 -geometry +6+6 -background '#C9CFDA' "$tmp/px.png"
convert -size 920x240 xc:'#C9CFDA' "$tmp/px.png" -gravity center -composite "$tmp/i-px.png"
# shellcheck disable=SC2086
convert $icons "$tmp/i-px.png" +append "$tmp/r6.png"

for r in 1 2 3 4 5; do
    convert "$tmp/r${r}a.png" "$tmp/r${r}b.png" +append "$tmp/r$r.png"
done
convert "$tmp/r1.png" "$tmp/r2.png" "$tmp/r3.png" "$tmp/r4.png" "$tmp/r5.png" \
    -background '#C9CFDA' -gravity west -append "$tmp/top.png"
convert "$tmp/top.png" "$tmp/r6.png" -background '#C9CFDA' -gravity west -append \
    "$out/logo-family.png"
rm -rf "$tmp"
echo "brand_logo_preview: $out/logo-family.png, $(ls "$png" | wc -l) PNGs in $png"
