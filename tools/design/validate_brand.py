#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 The SMY-OS Authors
"""Validate the brand assets (docs/BRAND_PROMPT.md §14).

Language: Python 3, standard library only (ADR 0002).

Checks:
  1. design/brand/BRAND_ASSET_MANIFEST.md lists every SVG in
     design/brand/logos/ and every listed file exists.
  2. The colours used by tools/design/brand_logo.py equal the tokens in
     design/tokens/brand-colors.json.
  3. Every documented contrast pair meets its minimum (WCAG 2.x formula).
  4. With --exports: build/brand/png/ holds every expected PNG export
     (run tools/design/brand_logo_preview.sh first).
SVG structure rules are checked by validate_svg.py.

Usage: tools/design/validate_brand.py [--exports]
"""

import json
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
LOGOS = ROOT / "design" / "brand" / "logos"
MANIFEST = ROOT / "design" / "brand" / "BRAND_ASSET_MANIFEST.md"
TOKENS = ROOT / "design" / "tokens" / "brand-colors.json"
EXPORTS = ROOT / "build" / "brand" / "png"

SQUARE_SIZES = (16, 24, 32, 48, 64, 128, 256, 512, 1024)
WIDE_SIZES = (480, 960, 1920)

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import brand_logo  # noqa: E402


def is_square(name):
    return name.startswith(("smy-symbol-", "smy-micro-", "smy-app-icon", "smy-boot-logo",
                            "smy-developer-mark"))


def luminance(hex_colour):
    def channel(c):
        c /= 255
        return c / 12.92 if c <= 0.03928 else ((c + 0.055) / 1.055) ** 2.4
    r, g, b = (int(hex_colour[i:i + 2], 16) for i in (1, 3, 5))
    return 0.2126 * channel(r) + 0.7152 * channel(g) + 0.0722 * channel(b)


def contrast(a, b):
    la, lb = sorted((luminance(a), luminance(b)), reverse=True)
    return (la + 0.05) / (lb + 0.05)


def lookup(tokens, name):
    if name.startswith("smy."):
        return tokens["brand"][name]
    group, key = name.split(".", 1)
    return tokens[group][key]


def main(argv):
    errors = []

    listed = set(re.findall(r"`(smy-[a-z0-9-]+\.svg)`", MANIFEST.read_text()))
    present = {p.name for p in LOGOS.glob("*.svg")}
    errors += [f"manifest lists missing file {n}" for n in sorted(listed - present)]
    errors += [f"{n} is not in the manifest" for n in sorted(present - listed)]

    tokens = json.loads(TOKENS.read_text())
    expected = {
        "VOID": tokens["brand"]["smy.void"], "CYAN": tokens["brand"]["smy.cyan"],
        "WHITE": tokens["brand"]["smy.white"], "MUTED": tokens["brand"]["smy.muted"],
        "LIGHT_BG": tokens["light"]["background"], "LIGHT_TEXT": tokens["light"]["text"],
        "LIGHT_MUTED": tokens["light"]["muted"], "LIGHT_ACCENT": tokens["light"]["accent"],
    }
    for const, value in expected.items():
        if getattr(brand_logo, const).upper() != value.upper():
            errors.append(f"brand_logo.{const} = {getattr(brand_logo, const)}, token says {value}")

    for pair in tokens["contrast"]["pairs"]:
        ratio = contrast(lookup(tokens, pair["fg"]), lookup(tokens, pair["bg"]))
        if ratio < pair["min"]:
            errors.append(f"contrast {pair['fg']} on {pair['bg']} = {ratio:.2f} < {pair['min']}")

    exports = 0
    if "--exports" in argv:
        for name in sorted(present):
            stem = name[:-4]
            for z in SQUARE_SIZES if is_square(stem) else WIDE_SIZES:
                exports += 1
                if not (EXPORTS / f"{stem}-{z}.png").is_file():
                    errors.append(f"missing export {stem}-{z}.png")

    if errors:
        for e in errors:
            print(f"validate_brand: FAIL: {e}")
        return 1
    print(f"validate_brand: OK ({len(present)} logos in manifest, "
          f"{len(tokens['contrast']['pairs'])} contrast pairs"
          + (f", {exports} PNG exports" if exports else "") + ")")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
