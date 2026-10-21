#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 The SMY-OS Authors
"""Generate the SMY-OS logo family from the approved "Module" symbol.

Language: Python 3, standard library only (ADR 0002).

Master geometry (ADR 0011, design/brand/BRAND_GUIDELINES.md): a 100-unit grid
with an 8-unit margin. The frame is a rounded square drawn with stroke S; its
top-right corner is left open and filled by the core, a separate square of
side 2*S, separated from the frame ends by S/2. One ratio system:

    S = 14 (stroke)   corner radius R = 20 (centre line)
    core = 2 S = 28   gap = S / 2 = 7   core radius = 6

The wordmark is the drawn SMY-OS wordmark of the concepts
(tools/design/brand_concepts.py), plus the few extra capitals needed for the
"INSTALLER" and "DEVELOPER" labels, drawn in the same monoline system.

Outputs: design/brand/logos/*.svg (committed). Usage:
    tools/design/brand_logo.py [--check]
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import brand_concepts as bc  # noqa: E402  (shared wordmark geometry)

ROOT = pathlib.Path(__file__).resolve().parents[2]
OUT = ROOT / "design" / "brand" / "logos"
f = bc.f

# Brand palette (docs/BRAND_PROMPT.md §4).
VOID = "#080B12"
CYAN = "#54F6E8"
WHITE = "#F4F7FF"
MUTED = "#929BAF"
LIGHT_BG = "#F5F7FB"
LIGHT_TEXT = "#141A29"
LIGHT_MUTED = "#5D687C"
LIGHT_ACCENT = "#087F83"
BLACK = "#000000"
PURE_WHITE = "#FFFFFF"

# ---------------------------------------------------------------- symbol


def module_symbol(stroke=14.0, radius=20.0, hollow_core=False):
    """Return [(kind, d, role, width)] for the Module symbol in a 100 box."""
    s = stroke
    lo, hi, r = 8 + s / 2, 92 - s / 2, radius
    core = 2 * s
    gap = s / 2
    k = core * 6 / 28  # core corner radius, 6 at the master size
    x_end, y_end = 92 - core - gap, 8 + core + gap
    frame = (
        f"M{f(x_end)} {f(lo)}H{f(lo + r)}A{f(r)} {f(r)} 0 0 0 {f(lo)} {f(lo + r)}"
        f"V{f(hi - r)}A{f(r)} {f(r)} 0 0 0 {f(lo + r)} {f(hi)}"
        f"H{f(hi - r)}A{f(r)} {f(r)} 0 0 0 {f(hi)} {f(hi - r)}V{f(y_end)}"
    )
    x0, y1 = 92 - core, 8 + core
    if hollow_core:  # developer mark: the core as an outline
        w = s / 2
        a, b, c, d = x0 + w / 2, 8 + w / 2, 92 - w / 2, y1 - w / 2
        kk = max(k - w / 2, 0.5)
        outline = (
            f"M{f(a + kk)} {f(b)}H{f(c - kk)}A{f(kk)} {f(kk)} 0 0 1 {f(c)} {f(b + kk)}"
            f"V{f(d - kk)}A{f(kk)} {f(kk)} 0 0 1 {f(c - kk)} {f(d)}H{f(a + kk)}"
            f"A{f(kk)} {f(kk)} 0 0 1 {f(a)} {f(d - kk)}V{f(b + kk)}A{f(kk)} {f(kk)} 0 0 1 {f(a + kk)} {f(b)}Z"
        )
        return [("stroke", frame, "main", s), ("stroke", outline, "accent", w)]
    core_d = (
        f"M{f(x0 + k)} 8H{f(92 - k)}A{f(k)} {f(k)} 0 0 1 92 {f(8 + k)}V{f(y1 - k)}"
        f"A{f(k)} {f(k)} 0 0 1 {f(92 - k)} {f(y1)}H{f(x0 + k)}A{f(k)} {f(k)} 0 0 1 {f(x0)} {f(y1 - k)}"
        f"V{f(8 + k)}A{f(k)} {f(k)} 0 0 1 {f(x0 + k)} 8Z"
    )
    return [("stroke", frame, "main", s), ("fill", core_d, "accent", 0)]


def micro_symbol():
    """Micro mark for 16-24 px: heavier stroke and smaller corner radius so
    the opening and the core survive pixel snapping."""
    return module_symbol(stroke=18.0, radius=14.0)


# ------------------------------------------------- extra label capitals
CAP, T = bc.CAP, bc.STROKE


def _x(x, w):
    return x + T / 2, x + w - T / 2, x + w / 2


def g_i(x, w):
    _, _, xm = _x(x, w)
    return f"M{f(xm)} 0V{f(CAP)}"


def g_n(x, w):
    x0, x1, _ = _x(x, w)
    return f"M{f(x0)} {f(CAP)}V0L{f(x1)} {f(CAP)}V0"


def g_t(x, w):
    _, _, xm = _x(x, w)
    return f"M{f(x)} {f(T / 2)}H{f(x + w)}M{f(xm)} {f(T / 2)}V{f(CAP)}"


def g_a(x, w):
    x0, x1, xm = _x(x, w)
    bar = CAP * 0.62
    return (f"M{f(x0)} {f(CAP)}L{f(xm)} {f(T / 2)}L{f(x1)} {f(CAP)}"
            f"M{f(x0 + (xm - x0) * (1 - bar / CAP) + T * 0.4)} {f(bar)}H{f(x1 - (x1 - xm) * (1 - bar / CAP) - T * 0.4)}")


def g_l(x, w):
    x0, _, _ = _x(x, w)
    return f"M{f(x0)} 0V{f(CAP - T / 2)}H{f(x + w)}"


def g_e(x, w):
    x0, _, _ = _x(x, w)
    return (f"M{f(x + w)} {f(T / 2)}H{f(x0)}V{f(CAP - T / 2)}H{f(x + w)}"
            f"M{f(x0)} {f(CAP / 2)}H{f(x + w - T)}")


def _bowl(x, w):
    """Upper bowl of P and R: rounded right corners with the S/O radius."""
    x0, x1, _ = _x(x, w)
    t, m = T / 2, CAP * 0.55
    r = min(bc.RADIUS, (m - t) / 2)
    return (f"M{f(x0)} {f(CAP)}V{f(t)}H{f(x1 - r)}A{f(r)} {f(r)} 0 0 1 {f(x1)} {f(t + r)}"
            f"V{f(m - r)}A{f(r)} {f(r)} 0 0 1 {f(x1 - r)} {f(m)}H{f(x0)}"), m, x0, x1


def g_p(x, w):
    return _bowl(x, w)[0]


def g_r(x, w):
    d, m, x0, x1 = _bowl(x, w)
    return d + f"M{f(x1 - 2 * bc.RADIUS)} {f(m)}L{f(x1)} {f(CAP)}"


def g_d(x, w):
    x0, x1, _ = _x(x, w)
    t, b, r = T / 2, CAP - T / 2, bc.RADIUS * 1.6
    return (f"M{f(x0)} {f(t)}H{f(x1 - r)}A{f(r)} {f(r)} 0 0 1 {f(x1)} {f(t + r)}"
            f"V{f(b - r)}A{f(r)} {f(r)} 0 0 1 {f(x1 - r)} {f(b)}H{f(x0)}Z")


def g_v(x, w):
    x0, x1, xm = _x(x, w)
    return f"M{f(x0)} 0L{f(xm)} {f(CAP - T / 2)}L{f(x1)} 0"


GLYPHS = {
    "S": (bc.letter_s, 58.0), "M": (bc.letter_m, 74.0), "Y": (bc.letter_y, 64.0),
    "-": (bc.letter_hyphen, 26.0), "O": (bc.letter_o, 64.0),
    "I": (g_i, 12.0), "N": (g_n, 62.0), "T": (g_t, 58.0), "A": (g_a, 66.0),
    "L": (g_l, 52.0), "E": (g_e, 54.0), "R": (g_r, 60.0), "P": (g_p, 58.0),
    "D": (g_d, 62.0), "V": (g_v, 64.0),
}


def text_path(text, tracking=bc.TRACK):
    x, parts = 0.0, []
    for ch in text:
        draw, w = GLYPHS[ch]
        parts.append(draw(x, w))
        x += w + tracking
    return "".join(parts), x - tracking


# ---------------------------------------------------------------- layouts
SYM = 100.0          # symbol box
WM_CAP = 46.0        # wordmark cap height in symbol units (46 % of the symbol)
WM_GAP = 34.0        # symbol -> wordmark distance
CLEAR = 28.0         # clear space = core size (2 S)
LABEL_CAP = 15.0     # INSTALLER / DEVELOPER label cap height
LABEL_TRACK = 40.0   # wide tracking for small labels (in glyph units)
STACK_CAP = 30.0     # stacked lockup: wordmark ~1.5x the symbol width
STACK_GAP = 26.0


def g_symbol(shapes, main, accent, x=0.0, y=0.0, scale=1.0):
    out = []
    for kind, d, role, width in shapes:
        colour = accent if role == "accent" else main
        if kind == "stroke":
            out.append(f'<path d="{d}" fill="none" stroke="{colour}" stroke-width="{f(width)}" '
                       f'stroke-linejoin="round"/>')
        else:
            out.append(f'<path d="{d}" fill="{colour}"/>')
    return f'<g transform="translate({f(x)} {f(y)}) scale({f(scale)})">{"".join(out)}</g>'


def g_text(text, colour, x, y, cap, tracking=bc.TRACK):
    d, w = text_path(text, tracking)
    s = cap / CAP
    stroke = bc.STROKE
    return (f'<g transform="translate({f(x)} {f(y)}) scale({f(s)})"><path d="{d}" fill="none" '
            f'stroke="{colour}" stroke-width="{f(stroke)}" stroke-linejoin="round"/></g>'), w * s


def doc(w, h, body, title, bg=None, radius=0.0):
    rect = ""
    if bg:
        rect = f'<rect width="{f(w)}" height="{f(h)}" rx="{f(radius)}" fill="{bg}"/>'
    return (f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {f(w)} {f(h)}" '
            f'width="{f(w)}" height="{f(h)}"><title>{title}</title>{rect}{body}</svg>\n')


def symbol_doc(shapes, main, accent, title):
    return doc(SYM, SYM, g_symbol(shapes, main, accent), title)


def horizontal(main, accent, title, label=None, label_colour=None):
    """Symbol + wordmark; optional small label under the wordmark."""
    shapes = module_symbol()
    _, wm_w = text_path("SMY-OS")
    wm_w *= WM_CAP / CAP
    w = CLEAR + SYM + WM_GAP + wm_w + CLEAR
    h = CLEAR + SYM + CLEAR
    wm_y = CLEAR + (SYM - WM_CAP) / 2
    if label:
        wm_y = CLEAR + 8.0  # wordmark rides higher to make room for the label
    body = g_symbol(shapes, main, accent, CLEAR, CLEAR)
    text, _ = g_text("SMY-OS", main, CLEAR + SYM + WM_GAP, wm_y, WM_CAP)
    body += text
    if label:
        lab, _ = g_text(label, label_colour, CLEAR + SYM + WM_GAP, CLEAR + SYM - 8.0 - LABEL_CAP,
                        LABEL_CAP, LABEL_TRACK)
        body += lab
    return doc(w, h, body, title)


def stacked(main, accent, title):
    shapes = module_symbol()
    _, wm_w = text_path("SMY-OS")
    wm_w *= STACK_CAP / CAP
    w = CLEAR + max(SYM, wm_w) + CLEAR
    h = CLEAR + SYM + STACK_GAP + STACK_CAP + CLEAR
    body = g_symbol(shapes, main, accent, (w - SYM) / 2, CLEAR)
    text, _ = g_text("SMY-OS", main, (w - wm_w) / 2, CLEAR + SYM + STACK_GAP, STACK_CAP)
    return doc(w, h, body + text, title)


def wordmark(colour, title):
    _, wm_w = text_path("SMY-OS")
    wm_w *= WM_CAP / CAP
    text, _ = g_text("SMY-OS", colour, CLEAR / 2, CLEAR / 2, WM_CAP)
    return doc(wm_w + CLEAR, WM_CAP + CLEAR, text, title)


def app_icon(tile, main, accent, title, micro=False):
    """Square app icon: a rounded tile (radius 22 % — superellipse-like)
    with the symbol at 62 % of the tile."""
    shapes = micro_symbol() if micro else module_symbol()
    scale = 0.62
    off = (100 - SYM * scale) / 2
    return doc(100, 100, g_symbol(shapes, main, accent, off, off, scale), title, tile, 22.0)


def generate():
    m = module_symbol()
    out = {
        # symbol
        "smy-symbol-on-dark.svg": symbol_doc(m, WHITE, CYAN, "SMY-OS symbol (for dark backgrounds)"),
        "smy-symbol-on-light.svg": symbol_doc(m, LIGHT_TEXT, LIGHT_ACCENT, "SMY-OS symbol (for light backgrounds)"),
        "smy-symbol-mono-black.svg": symbol_doc(m, BLACK, BLACK, "SMY-OS symbol (monochrome black)"),
        "smy-symbol-mono-white.svg": symbol_doc(m, PURE_WHITE, PURE_WHITE, "SMY-OS symbol (monochrome white)"),
        # micro mark
        "smy-micro-on-dark.svg": symbol_doc(micro_symbol(), WHITE, CYAN, "SMY-OS micro mark (16-24 px, dark)"),
        "smy-micro-on-light.svg": symbol_doc(micro_symbol(), LIGHT_TEXT, LIGHT_ACCENT, "SMY-OS micro mark (16-24 px, light)"),
        # wordmark
        "smy-wordmark-on-dark.svg": wordmark(WHITE, "SMY-OS wordmark (for dark backgrounds)"),
        "smy-wordmark-on-light.svg": wordmark(LIGHT_TEXT, "SMY-OS wordmark (for light backgrounds)"),
        # master lockups
        "smy-logo-horizontal-on-dark.svg": horizontal(WHITE, CYAN, "SMY-OS logo (horizontal, dark backgrounds)"),
        "smy-logo-horizontal-on-light.svg": horizontal(LIGHT_TEXT, LIGHT_ACCENT, "SMY-OS logo (horizontal, light backgrounds)"),
        "smy-logo-horizontal-mono-black.svg": horizontal(BLACK, BLACK, "SMY-OS logo (horizontal, monochrome black)"),
        "smy-logo-horizontal-mono-white.svg": horizontal(PURE_WHITE, PURE_WHITE, "SMY-OS logo (horizontal, monochrome white)"),
        "smy-logo-stacked-on-dark.svg": stacked(WHITE, CYAN, "SMY-OS logo (stacked, dark backgrounds)"),
        "smy-logo-stacked-on-light.svg": stacked(LIGHT_TEXT, LIGHT_ACCENT, "SMY-OS logo (stacked, light backgrounds)"),
        # app icon
        "smy-app-icon.svg": app_icon(VOID, WHITE, CYAN, "SMY-OS app icon"),
        "smy-app-icon-light.svg": app_icon(PURE_WHITE, LIGHT_TEXT, LIGHT_ACCENT, "SMY-OS app icon (light)"),
        "smy-app-icon-small.svg": app_icon(VOID, WHITE, CYAN, "SMY-OS app icon (16-24 px)", micro=True),
        # boot and installer
        "smy-boot-logo.svg": symbol_doc(m, WHITE, CYAN, "SMY-OS boot logo (on #080B12)"),
        "smy-installer-logo-on-dark.svg": horizontal(WHITE, CYAN, "SMY-OS installer logo", "INSTALLER", MUTED),
        "smy-installer-logo-on-light.svg": horizontal(LIGHT_TEXT, LIGHT_ACCENT, "SMY-OS installer logo (light)",
                                                      "INSTALLER", LIGHT_MUTED),
        # developer mark
        "smy-developer-mark-on-dark.svg": symbol_doc(module_symbol(hollow_core=True), WHITE, CYAN,
                                                     "SMY-OS developer mark"),
        "smy-developer-logo-on-dark.svg": horizontal(WHITE, CYAN, "SMY-OS developer logo", "DEVELOPER", MUTED),
    }
    return out


def main(argv):
    files = generate()
    if "--check" in argv:
        stale = [n for n, c in files.items() if not (OUT / n).exists() or (OUT / n).read_text() != c]
        extra = [p.name for p in OUT.glob("*.svg") if p.name not in files] if OUT.exists() else []
        if stale or extra:
            print("brand_logo: out of date: " + ", ".join(stale + extra), file=sys.stderr)
            return 1
        print(f"brand_logo: {len(files)} SVGs up to date")
        return 0
    OUT.mkdir(parents=True, exist_ok=True)
    for name, content in files.items():
        (OUT / name).write_text(content)
    print(f"brand_logo: wrote {len(files)} SVGs to {OUT.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
