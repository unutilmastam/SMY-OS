#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 The SMY-OS Authors
"""Render a review sheet of the three themes from the design tokens.

Language: Python 3, standard library only (ADR 0002). Writes an SVG (with
text in Inter) that tools/design/theme_preview.sh turns into a PNG. It is a
reference sheet for review, not a component design: components are
specified in Phase 17 (D3).

Usage: tools/design/theme_preview.py <out.svg>
"""

import json
import pathlib
import sys
from xml.sax.saxutils import escape

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import check_contrast  # noqa: E402

ROOT = pathlib.Path(__file__).resolve().parents[2]
F = json.loads((ROOT / "design" / "tokens" / "foundations.json").read_text())
SP = {k: v for k, v in F["space"].items() if not k.startswith("$")}
R = F["radius"]

COL_W, PAD = 560, 40
SYMBOL = ("M57 15H35A20 20 0 0 0 15 35V65A20 20 0 0 0 35 85H65A20 20 0 0 0 85 65V43",
          "M70 8H86A6 6 0 0 1 92 14V30A6 6 0 0 1 86 36H70A6 6 0 0 1 64 30V14A6 6 0 0 1 70 8Z")


def text(x, y, s, colour, style="body", anchor="start", family="ui"):
    t = F["type"][style]
    w = F["font"]["weight"][t["weight"]]
    fam = F["font"]["family"][family]
    return (f'<text x="{x}" y="{y}" fill="{colour}" font-family="{fam}" font-size="{t["size"]}" '
            f'font-weight="{w}" letter-spacing="{t["tracking"] / 1000 * t["size"]:.2f}" '
            f'text-anchor="{anchor}">{escape(s)}</text>')


def rect(x, y, w, h, fill, r=0, stroke=None, sw=1):
    s = f' stroke="{stroke}" stroke-width="{sw}"' if stroke else ""
    return f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="{r}" fill="{fill}"{s}/>'


def column(x0, name, label, c, hc):
    o = []
    H = 880
    o.append(rect(x0, 0, COL_W, H, c["bg-app"]))
    x, y = x0 + PAD, PAD
    o.append(text(x, y + 14, label.upper(), c["text-secondary"], "label"))
    y += 36

    # Window
    ww, wh = COL_W - 2 * PAD, 560
    border = c["border-strong"] if hc else c["border-subtle"]
    o.append(rect(x, y, ww, wh, c["surface"], R["window"], border, 2 if hc else 1))
    # Title area with the symbol
    o.append(f'<g transform="translate({x + 16} {y + 12}) scale(0.16)">'
             f'<path d="{SYMBOL[0]}" fill="none" stroke="{c["text-primary"]}" stroke-width="14" '
             f'stroke-linejoin="round"/><path d="{SYMBOL[1]}" fill="{c["accent"]}"/></g>')
    o.append(text(x + 44, y + 26, "Sozlamalar", c["text-primary"], "body-strong"))
    # Window controls: small rounded squares (the logo's module shape); the
    # real control design is Phase 17 (D3).
    for i in range(3):
        cx = x + ww - 30 - i * 26
        o.append(rect(cx, y + 15, 12, 12, "none", 3.5, c["text-secondary"], 1.5))
    o.append(f'<line x1="{x}" y1="{y + 44}" x2="{x + ww}" y2="{y + 44}" stroke="{border}"/>')

    # Sidebar (sunken) with a selected row
    sx, sy = x + 12, y + 56
    o.append(rect(sx, sy, 150, wh - 68, c["surface-sunken"], R["panel"]))
    rows = ["Tizim", "Ekran", "Tarmoq", "Xavfsizlik", "Til: Oʻzbekcha"]
    for i, r in enumerate(rows):
        ry = sy + 12 + i * 36
        if i == 1:
            o.append(rect(sx + 8, ry, 134, 30, c["accent-subtle"], R["control"]))
            o.append(rect(sx + 8, ry + 8, 3, 14, c["accent"], 1.5))
        o.append(text(sx + 20, ry + 20, r, c["text-primary"] if i == 1 else c["text-secondary"],
                      "body-small"))

    # Content
    cx, cy = sx + 170, sy + 6
    o.append(text(cx, cy + 24, "Ekran", c["text-primary"], "title-large"))
    o.append(text(cx, cy + 50, "Yorqinlik, oʻlcham va tema.", c["text-secondary"], "body"))
    # Theme setting: segmented
    o.append(text(cx, cy + 92, "Tema", c["text-primary"], "label"))
    seg_w = 90
    for i, s in enumerate(["Qorongʻi", "Yorugʻ", "Kontrast"]):
        bx = cx + i * (seg_w + 6)
        active = (name == ["dark", "light", "high-contrast"][i])
        o.append(rect(bx, cy + 104, seg_w, 32, c["accent"] if active else c["surface-sunken"],
                      R["control"], None if active else c["border-strong"]))
        o.append(text(bx + seg_w / 2, cy + 125, s, c["text-on-accent"] if active else c["text-primary"],
                      "label", "middle"))
    # Input with focus ring
    o.append(text(cx, cy + 172, "Qurilma nomi", c["text-primary"], "label"))
    fr = F["border"]["focus-ring-high-contrast" if hc else "focus-ring"]
    off = F["border"]["focus-offset"]
    iw = ww - 170 - 24
    o.append(rect(cx - off - fr / 2, cy + 184 - off - fr / 2, iw + 2 * off + fr, 36 + 2 * off + fr,
                  "none", R["control"] + off, c["focus-ring"], fr))
    o.append(rect(cx, cy + 184, iw, 36, c["surface-sunken"], R["control"], c["border-strong"]))
    o.append(text(cx + 12, cy + 207, "smy-noutbuk", c["text-primary"], "body"))
    o.append(f'<line x1="{cx + 96}" y1="{cy + 193}" x2="{cx + 96}" y2="{cy + 211}" '
             f'stroke="{c["accent"]}" stroke-width="1.5"/>')
    # Slider
    o.append(text(cx, cy + 254, "Yorqinlik", c["text-primary"], "label"))
    o.append(text(cx + iw, cy + 254, "72%", c["text-secondary"], "label", "end"))
    o.append(rect(cx, cy + 268, iw, 4, c["border-subtle"] if not hc else c["border-strong"], 2))
    o.append(rect(cx, cy + 268, iw * 0.72, 4, c["accent"], 2))
    o.append(f'<circle cx="{cx + iw * 0.72}" cy="{cy + 270}" r="8" fill="{c["text-primary"] if hc else c["surface-raised"]}" '
             f'stroke="{c["accent"]}" stroke-width="2"/>')
    # Buttons
    by = cy + 310
    o.append(rect(cx, by, 112, 32, c["accent"], R["control"]))
    o.append(text(cx + 56, by + 21, "Saqlash", c["text-on-accent"], "label", "middle"))
    o.append(rect(cx + 122, by, 112, 32, c["surface"], R["control"], c["border-strong"]))
    o.append(text(cx + 178, by + 21, "Bekor qilish", c["text-primary"], "label", "middle"))
    o.append(text(cx, by + 64, "Oʻzgarishlar avtomatik saqlanadi.", c["text-disabled"], "caption"))
    o.append(text(cx, by + 92, "ls -la /system", c["text-secondary"], "mono", family="mono"))

    # Banners
    y = y + wh + 24
    for kind, msg in [("success", "Yangilanish oʻrnatildi"), ("warning", "Batareya 15%"),
                      ("danger", "Disk xatosi topildi"), ("info", "Yangi versiya mavjud")]:
        o.append(rect(x, y, ww, 40, c[f"{kind}-surface"], R["control"],
                      c[kind] if hc else None, 1))
        o.append(f'<circle cx="{x + 20}" cy="{y + 20}" r="5" fill="{c[kind]}"/>')
        o.append(text(x + 36, y + 25, msg, c["text-primary"], "body"))
        o.append(text(x + ww - 16, y + 25, kind, c[kind], "label", "end"))
        y += 48
    return "".join(o)


def ramps(y0, width, base):
    o = [rect(0, y0, width, 210, "#FFFFFF")]
    hues = ["neutral", "cyan", "violet", "green", "amber", "red", "blue"]
    steps = [k.split(".")[1] for k in base if k.startswith("neutral.")]
    cw = (width - 2 * PAD - 80) / len(steps)
    for i, s in enumerate(steps):
        o.append(text(PAD + 80 + i * cw + cw / 2, y0 + 24, s, "#5D687C", "caption", "middle"))
    for j, h in enumerate(hues):
        yy = y0 + 32 + j * 24
        o.append(text(PAD, yy + 16, h, "#141A29", "caption"))
        for i, s in enumerate(steps):
            o.append(rect(PAD + 80 + i * cw, yy, cw - 2, 20, base[f"{h}.{s}"], 3))
    return "".join(o)


def main(argv):
    themes, data = check_contrast.resolve_themes()
    base = check_contrast.load_base()
    order = ["dark", "light", "high-contrast"]
    width = COL_W * len(order)
    body = []
    for i, name in enumerate(order):
        body.append(column(i * COL_W, name, data["themes"][name]["label"], themes[name],
                           name == "high-contrast"))
    body.append(ramps(880, width, base))
    svg = (f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="1090" '
           f'viewBox="0 0 {width} 1090">{"".join(body)}</svg>\n')
    pathlib.Path(argv[0]).write_text(svg)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
