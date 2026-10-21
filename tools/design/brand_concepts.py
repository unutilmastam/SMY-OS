#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 The SMY-OS Authors
"""Generate the SMY-OS logo concepts (Phase 5B step 1) as SVG.

Language: Python 3, standard library only (ADR 0002).

Every shape is constructed from documented numbers on a 100-unit grid, so the
geometry is exact and can be refined by changing parameters, not by hand
editing. Outputs (committed, reviewed design sources):

    design/brand/concepts/concept-<id>-symbol.svg      symbol, dark-on-transparent
    design/brand/concepts/concept-<id>-lockup-dark.svg symbol + wordmark on dark
    design/brand/concepts/concept-<id>-lockup-light.svg
    design/brand/concepts/concept-<id>-mono.svg        one colour

Usage: tools/design/brand_concepts.py [--check]
  --check  fail if the committed SVGs differ from what this script generates
"""

import math
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
OUT = ROOT / "design" / "brand" / "concepts"

# Owner-provided brand palette (docs/BRAND_PROMPT.md §4).
VOID = "#080B12"
CYAN = "#54F6E8"
WHITE = "#F4F7FF"
LIGHT_BG = "#F5F7FB"
LIGHT_TEXT = "#141A29"
LIGHT_ACCENT = "#087F83"


def f(v):
    """Format a coordinate compactly and deterministically."""
    s = f"{v:.3f}".rstrip("0").rstrip(".")
    return "0" if s in ("-0", "") else s


# ---------------------------------------------------------------- wordmark
# Monoline geometric letters drawn on their centre lines. Cap height CAP,
# stroke STROKE; centre lines are inset by STROKE/2 so the outer edge of the
# stroke sits exactly on the cap height and baseline.
CAP = 80.0
STROKE = 12.0
RADIUS = 17.0  # one corner radius for S and O: the wordmark's signature
TRACK = 14.0   # space between letter boxes


def letter_s(x, w):
    t, b = STROKE / 2, CAP - STROKE / 2
    m = CAP / 2
    x0, x1 = x + STROKE / 2, x + w - STROKE / 2
    r = (m - t) / 2
    return (
        f"M{f(x1)} {f(t)}H{f(x0 + r)}"
        f"A{f(r)} {f(r)} 0 0 0 {f(x0)} {f(t + r)}"
        f"A{f(r)} {f(r)} 0 0 0 {f(x0 + r)} {f(m)}"
        f"H{f(x1 - r)}"
        f"A{f(r)} {f(r)} 0 0 1 {f(x1)} {f(m + r)}"
        f"A{f(r)} {f(r)} 0 0 1 {f(x1 - r)} {f(b)}"
        f"H{f(x0)}"
    )


def letter_m(x, w):
    t, b = STROKE / 2, CAP
    x0, x1 = x + STROKE / 2, x + w - STROKE / 2
    xm = x + w / 2
    # The middle vertex sits at 62 % of the cap height.
    return f"M{f(x0)} {f(b)}V{f(t)}L{f(xm)} {f(CAP * 0.62)}L{f(x1)} {f(t)}V{f(b)}"


def letter_y(x, w):
    t, b = 0.0, CAP
    x0, x1 = x + STROKE / 2, x + w - STROKE / 2
    xm = x + w / 2
    junction = CAP * 0.5
    return f"M{f(x0)} {f(t)}L{f(xm)} {f(junction)}L{f(x1)} {f(t)}M{f(xm)} {f(junction)}V{f(b)}"


def letter_hyphen(x, w):
    m = CAP / 2
    return f"M{f(x)} {f(m)}H{f(x + w)}"


def letter_o(x, w):
    t, b = STROKE / 2, CAP - STROKE / 2
    x0, x1 = x + STROKE / 2, x + w - STROKE / 2
    r = RADIUS
    return (
        f"M{f(x0 + r)} {f(t)}H{f(x1 - r)}A{f(r)} {f(r)} 0 0 1 {f(x1)} {f(t + r)}"
        f"V{f(b - r)}A{f(r)} {f(r)} 0 0 1 {f(x1 - r)} {f(b)}H{f(x0 + r)}"
        f"A{f(r)} {f(r)} 0 0 1 {f(x0)} {f(b - r)}V{f(t + r)}"
        f"A{f(r)} {f(r)} 0 0 1 {f(x0 + r)} {f(t)}Z"
    )


WORDMARK = [  # (drawer, box width)
    (letter_s, 58.0),
    (letter_m, 74.0),
    (letter_y, 64.0),
    (letter_hyphen, 26.0),
    (letter_o, 64.0),
    (letter_s, 58.0),
]


def wordmark_paths():
    """Return (path data, total width) of 'SMY-OS'."""
    x = 0.0
    parts = []
    for drawer, w in WORDMARK:
        parts.append(drawer(x, w))
        x += w + TRACK
    return "".join(parts), x - TRACK


# ----------------------------------------------------------------- symbols
# Symbols live in a 100 x 100 box. Each returns a list of
# (kind, data, role) where kind is "stroke" (with width) or "fill", and role
# is "main" or "accent".


def symbol_portal():
    """A: Module — a rounded-square frame whose top-right corner is not drawn;
    the computational core sits in that corner as a separate square, so the
    frame is completed by the core ("modular architecture")."""
    s = 14.0
    lo, hi, r = 8 + s / 2, 92 - s / 2, 20.0
    cut = 33.0  # distance from the outer edge where the frame stops
    frame = (
        f"M{f(92 - cut)} {f(lo)}H{f(lo + r)}A{f(r)} {f(r)} 0 0 0 {f(lo)} {f(lo + r)}"
        f"V{f(hi - r)}A{f(r)} {f(r)} 0 0 0 {f(lo + r)} {f(hi)}"
        f"H{f(hi - r)}A{f(r)} {f(r)} 0 0 0 {f(hi)} {f(hi - r)}V{f(8 + cut)}"
    )
    c = 23.0   # core square, outer edges on the frame's outer edges
    x0, y1 = 92 - c, 8 + c
    k = 5.0    # small corner radius of the core
    core = (
        f"M{f(x0 + k)} 8H{f(92 - k)}A{f(k)} {f(k)} 0 0 1 92 {f(8 + k)}V{f(y1 - k)}"
        f"A{f(k)} {f(k)} 0 0 1 {f(92 - k)} {f(y1)}H{f(x0 + k)}A{f(k)} {f(k)} 0 0 1 {f(x0)} {f(y1 - k)}"
        f"V{f(8 + k)}A{f(k)} {f(k)} 0 0 1 {f(x0 + k)} 8Z"
    )
    return [("stroke", frame, "main", s), ("fill", core, "accent", 0)]


def symbol_signal():
    """B: Signal — one continuous path tracing an S on an orthogonal grid,
    starting from a node (the core) and ending in an open terminal."""
    s = 14.0
    lo, hi = 8 + s / 2, 92 - s / 2
    m = 50.0
    r = (m - lo) / 2
    path = (
        f"M{f(hi)} {f(lo)}H{f(lo + r)}A{f(r)} {f(r)} 0 0 0 {f(lo)} {f(lo + r)}"
        f"A{f(r)} {f(r)} 0 0 0 {f(lo + r)} {f(m)}H{f(hi - r)}"
        f"A{f(r)} {f(r)} 0 0 1 {f(hi)} {f(m + r)}A{f(r)} {f(r)} 0 0 1 {f(hi - r)} {f(hi)}"
        f"H{f(lo + 4)}"
    )
    node_r = 11.0  # the node is wider than the stroke and sits on the path's end
    nx = lo + 4
    node = (
        f"M{f(nx - node_r)} {f(hi)}a{f(node_r)} {f(node_r)} 0 1 0 {f(2 * node_r)} 0"
        f"a{f(node_r)} {f(node_r)} 0 1 0 {f(-2 * node_r)} 0Z"
    )
    return [("stroke", path, "main", s), ("fill", node, "accent", 0)]


def symbol_prism():
    """C: Prism — a hexagonal frame whose three inner arms form a Y (and an
    isometric corner), meeting at a hexagonal core."""
    s = 9.0
    cx, cy, R = 50.0, 50.0, 42.0 - s / 2

    def pt(angle_deg, radius):
        a = math.radians(angle_deg)
        return cx + radius * math.cos(a), cy - radius * math.sin(a)

    angles = [90, 30, -30, -90, -150, 150]
    hexagon = "M" + "L".join(f"{f(x)} {f(y)}" for x, y in (pt(a, R) for a in angles)) + "Z"
    core_r = 9.0
    arms = ""
    for a in (150, 30, -90):  # Y: upper-left, upper-right, down
        x0, y0 = pt(a, core_r + 5)
        x1, y1 = pt(a, R)
        arms += f"M{f(x0)} {f(y0)}L{f(x1)} {f(y1)}"
    core = "M" + "L".join(f"{f(x)} {f(y)}" for x, y in (pt(a, core_r) for a in angles)) + "Z"
    return [("stroke", hexagon, "main", s), ("stroke", arms, "main", s), ("fill", core, "accent", 0)]


CONCEPTS = {
    "a": ("Module", symbol_portal),
    "b": ("Signal", symbol_signal),
    "c": ("Prism", symbol_prism),
}


# ---------------------------------------------------------------- output
def shapes_svg(shapes, main, accent):
    out = []
    for kind, data, role, width in shapes:
        colour = accent if role == "accent" else main
        if kind == "stroke":
            out.append(
                f'<path d="{data}" fill="none" stroke="{colour}" stroke-width="{f(width)}" '
                f'stroke-linecap="butt" stroke-linejoin="round"/>'
            )
        else:
            out.append(f'<path d="{data}" fill="{colour}"/>')
    return "".join(out)


def svg_doc(width, height, body, title, background=None):
    bg = f'<rect width="{f(width)}" height="{f(height)}" fill="{background}"/>' if background else ""
    return (
        f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {f(width)} {f(height)}" '
        f'width="{f(width)}" height="{f(height)}">'
        f"<title>{title}</title>{bg}{body}</svg>\n"
    )


def lockup(shapes, main, accent, background, title):
    """Horizontal lockup: symbol (100 units) + gap + wordmark scaled to 0.5 of
    the symbol height... (wordmark cap height = 46 % of symbol height)."""
    wm, wm_w = wordmark_paths()
    pad = 40.0
    sym = 100.0
    scale = 46.0 / CAP
    gap = 34.0
    width = pad + sym + gap + wm_w * scale + pad
    height = sym + 2 * pad
    body = (
        f'<g transform="translate({f(pad)} {f(pad)})">{shapes_svg(shapes, main, accent)}</g>'
        f'<g transform="translate({f(pad + sym + gap)} {f(pad + (sym - CAP * scale) / 2)}) scale({f(scale)})">'
        f'<path d="{wm}" fill="none" stroke="{main}" stroke-width="{f(STROKE)}" '
        f'stroke-linecap="butt" stroke-linejoin="round"/></g>'
    )
    return svg_doc(width, height, body, title, background)


def generate():
    files = {}
    for cid, (name, make) in CONCEPTS.items():
        shapes = make()
        title = f"SMY-OS logo concept {cid.upper()}: {name}"
        files[f"concept-{cid}-symbol.svg"] = svg_doc(100, 100, shapes_svg(shapes, WHITE, CYAN), title)
        files[f"concept-{cid}-lockup-dark.svg"] = lockup(shapes, WHITE, CYAN, VOID, title + " (dark)")
        files[f"concept-{cid}-lockup-light.svg"] = lockup(
            shapes, LIGHT_TEXT, LIGHT_ACCENT, LIGHT_BG, title + " (light)")
        files[f"concept-{cid}-mono.svg"] = lockup(shapes, LIGHT_TEXT, LIGHT_TEXT, None, title + " (monochrome)")
    return files


def main(argv):
    files = generate()
    if "--check" in argv:
        stale = [n for n, c in files.items() if not (OUT / n).exists() or (OUT / n).read_text() != c]
        if stale:
            print("brand_concepts: out of date: " + ", ".join(stale), file=sys.stderr)
            return 1
        print(f"brand_concepts: {len(files)} SVGs up to date")
        return 0
    OUT.mkdir(parents=True, exist_ok=True)
    for name, content in files.items():
        (OUT / name).write_text(content)
    print(f"brand_concepts: wrote {len(files)} SVGs to {OUT.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
