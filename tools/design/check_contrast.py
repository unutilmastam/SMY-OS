#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 The SMY-OS Authors
"""Automated WCAG 2.2 contrast test for every theme (DESIGN_SYSTEM.md).

Language: Python 3, standard library only (ADR 0002).

For each theme in design/tokens/themes.json, every pair listed under
"pairs" must reach the theme's minimum for its kind (text 4.5:1, large text
and UI boundaries 3:1; high contrast 7:1 / 4.5:1). Also checks that every
theme defines exactly the same semantic tokens, that every value resolves to
a base or brand colour, and that exempt tokens (disabled text, decorative
dividers) still reach their own documented floor.

Usage: tools/design/check_contrast.py [--report]   (--report prints every ratio)
"""

import json
import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import colour  # noqa: E402

ROOT = pathlib.Path(__file__).resolve().parents[2]
TOKENS = ROOT / "design" / "tokens"


def load_base():
    base = {}
    for hue, ramp in json.loads((TOKENS / "color-base.json").read_text()).items():
        if hue.startswith("$"):
            continue
        for step, value in ramp.items():
            base[f"{hue}.{step}"] = value
    brand = json.loads((TOKENS / "brand-colors.json").read_text())["brand"]
    base.update(brand)
    return base


def resolve_themes():
    """{theme: {token: '#RRGGBB'}}, plus the raw theme data; raises on bad refs."""
    base = load_base()
    data = json.loads((TOKENS / "themes.json").read_text())
    resolved = {}
    for name, theme in data["themes"].items():
        out = {}
        for token, ref in theme["colors"].items():
            if ref not in base:
                raise SystemExit(f"check_contrast: {name}.{token} = {ref!r} is not a base or brand colour")
            out[token] = base[ref]
        resolved[name] = out
    return resolved, data


def main(argv):
    report = "--report" in argv
    resolved, data = resolve_themes()
    errors = []
    checked = 0

    names = {n: set(t) for n, t in resolved.items()}
    first = next(iter(names.values()))
    for n, tokens in names.items():
        if tokens != first:
            errors.append(f"theme {n} defines a different token set: {sorted(tokens ^ first)}")

    for name, theme in data["themes"].items():
        colours = resolved[name]
        for pair in data["pairs"]:
            minimum = theme["minimums"][pair["kind"]]
            for bg in pair["bg"]:
                ratio = colour.contrast(colours[pair["fg"]], colours[bg])
                checked += 1
                if report:
                    print(f"  {name:13} {pair['fg']:15} on {bg:16} {ratio:5.2f}  (min {minimum})")
                if ratio < minimum - 1e-9:
                    errors.append(f"{name}: {pair['fg']} on {bg} = {ratio:.2f}:1 < {minimum}:1 ({pair['kind']})")
        for token, rule in data["exempt"].items():
            for bg in ("bg-app", "surface", "surface-raised"):
                ratio = colour.contrast(colours[token], colours[bg])
                if ratio < rule["min"]:
                    errors.append(f"{name}: exempt {token} on {bg} = {ratio:.2f}:1 < {rule['min']}:1")

    if errors:
        for e in errors:
            print(f"check_contrast: FAIL {e}")
        return 1
    print(f"check_contrast: OK ({len(resolved)} themes, {checked} pairs)")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
