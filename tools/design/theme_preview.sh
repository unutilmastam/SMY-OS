#!/bin/sh
# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 The SMY-OS Authors
#
# Render the theme review sheet: build/brand/themes-preview.png.
# Text uses the pinned fonts from tools/fetch-fonts.sh through a private
# fontconfig file, so the sheet looks the same locally and in CI.
# Language: POSIX sh (ADR 0002). Tools: rsvg-convert (librsvg).

set -eu

root=$(cd "$(dirname "$0")/../.." && pwd)
out="$root/build/brand"
fonts="$root/third_party/downloads/fonts"
if [ ! -f "$fonts/inter/Inter-Regular.ttf" ]; then
    echo "theme_preview: fonts missing; run tools/fetch-fonts.sh" >&2
    exit 1
fi
mkdir -p "$out"
cat >"$out/fonts.conf" <<CONF
<?xml version="1.0"?>
<!DOCTYPE fontconfig SYSTEM "fonts.dtd">
<fontconfig>
  <dir>$fonts</dir>
  <cachedir>$out/fontconfig-cache</cachedir>
</fontconfig>
CONF
python3 "$root/tools/design/theme_preview.py" "$out/themes-preview.svg"
FONTCONFIG_FILE="$out/fonts.conf" rsvg-convert "$out/themes-preview.svg" -o "$out/themes-preview.png"
echo "theme_preview: $out/themes-preview.png"
