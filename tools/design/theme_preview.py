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

COL_W, PAD = 600, 48
SYMBOL = ("M57 15H35A20 20 0 0 0 15 35V65A20 20 0 0 0 35 85H65A20 20 0 0 0 85 65V43",
          "M70 8H86A6 6 0 0 1 92 14V30A6 6 0 0 1 86 36H70A6 6 0 0 1 64 30V14A6 6 0 0 1 70 8Z")


def text(x, y, s, colour, style="body", anchor="start", family="ui"):
    t = F["type"][style]
    w = F["font"]["weight"][t["weight"]]
    fam = F["font"]["family"][family]
    return (f'<text x="{x}" y="{y}" fill="{colour}" font-family="{fam}" font-size="{t["size"]}" '
            f'font-weight="{w}" letter-spacing="{t["tracking"] / 1000 * t["size"]:.2f}" '
            f'text-anchor="{anchor}">{escape(s)}</text>')


def rect(x, y, w, h, fill, r=0, stroke=None, sw=1.0):
    s = f' stroke="{stroke}" stroke-width="{sw}"' if stroke else ""
    return f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="{r}" fill="{fill}"{s}/>'


def line(x1, y1, x2, y2, colour, w=1.0):
    return (f'<line x1="{x1}" y1="{y1}" x2="{x2}" y2="{y2}" stroke="{colour}" stroke-width="{w}" '
            f'stroke-linecap="round"/>')


def symbol(x, y, size, main, accent):
    k = size / 100
    return (f'<g transform="translate({x} {y}) scale({k})"><path d="{SYMBOL[0]}" fill="none" '
            f'stroke="{main}" stroke-width="14" stroke-linejoin="round"/>'
            f'<path d="{SYMBOL[1]}" fill="{accent}"/></g>')


def column(x0, name, label, c, hc):
    o = []
    H = 660
    o.append(rect(x0, 0, COL_W, H, c["bg-app"]))
    x, y = x0 + PAD, PAD
    o.append(text(x, y + 12, label.upper(), c["text-secondary"], "caption"))
    y += 32

    ww, wh = COL_W - 2 * PAD, 470
    hair = c["border-strong"] if hc else c["border-subtle"]
    sw = 2 if hc else 1
    # Window: one quiet frame, a soft single shadow (not in high contrast)
    shadow = "" if hc else ' filter="url(#elev3)"'
    o.append(rect(x, y, ww, wh, c["surface"], R["window"], hair, sw).replace("/>", shadow + "/>"))
    # Title bar: symbol, title, minimal window controls (D3 designs them)
    o.append(symbol(x + 18, y + 14, 16, c["text-primary"], c["accent"]))
    o.append(text(x + 44, y + 27, "Sozlamalar", c["text-primary"], "label"))
    gx = x + ww - 22
    o.append(line(gx - 5, y + 17, gx + 5, y + 27, c["text-secondary"], 1.4))
    o.append(line(gx + 5, y + 17, gx - 5, y + 27, c["text-secondary"], 1.4))
    o.append(rect(gx - 37, y + 17, 10, 10, "none", 2, c["text-secondary"], 1.4))
    o.append(line(gx - 69, y + 22, gx - 59, y + 22, c["text-secondary"], 1.4))
    o.append(line(x, y + 44, x + ww, y + 44, hair, sw))

    # Sidebar
    sx, sy, sw_ = x, y + 44, 156
    o.append(f'<path d="M{sx} {sy}H{sx + sw_}V{y + wh}H{sx + R["window"]}A{R["window"]} {R["window"]} 0 0 1 {sx} {y + wh - R["window"]}Z" '
             f'fill="{c["surface-sunken"]}"/>')
    o.append(line(sx + sw_, sy, sx + sw_, y + wh, hair, sw))
    for i, r in enumerate(["Tizim", "Ekran", "Tovush", "Tarmoq", "Xavfsizlik", "Til va hudud"]):
        ry = sy + 16 + i * 34
        if i == 1:
            o.append(rect(sx + 10, ry, sw_ - 20, 28, c["surface-selected"], R["control"]))
            o.append(rect(sx + 10, ry + 8, 2, 12, c["accent"], 1))
        o.append(text(sx + 24, ry + 19, r, c["text-primary"] if i == 1 else c["text-secondary"],
                      "body-small"))

    # Content
    cx = sx + sw_ + 28
    cw = x + ww - 28 - cx
    cy = sy + 40
    o.append(text(cx, cy, "Ekran", c["text-primary"], "title"))
    o.append(text(cx, cy + 24, "Yorqinlik, oʻlcham va koʻrinish", c["text-secondary"], "body-small"))
    rows_y = cy + 52
    row_h = 60

    def row(i, title, sub=None):
        ry = rows_y + i * row_h
        if i > 0:
            o.append(line(cx, ry, cx + cw, ry, hair, 1))
        o.append(text(cx, ry + (26 if sub else 35), title, c["text-primary"], "body"))
        if sub:
            o.append(text(cx, ry + 44, sub, c["text-secondary"], "caption"))
        return ry

    # 1. Theme: neutral segmented control
    ry = row(0, "Tema")
    seg = ["Qorongʻi", "Yorugʻ", "Kontrast"]
    active = ["dark", "light", "high-contrast"].index(name)
    segw = 72
    sx0 = cx + cw - 3 * segw - 4
    o.append(rect(sx0, ry + 14, 3 * segw + 4, 32, c["surface-sunken"], R["control"] + 2, hair, sw))
    for i, s_ in enumerate(seg):
        bx = sx0 + 2 + i * segw
        if i == active:
            o.append(rect(bx, ry + 16, segw, 28, c["surface-raised"], R["control"],
                          c["border-strong"] if hc else hair, sw))
        o.append(text(bx + segw / 2, ry + 34, s_, c["text-primary"] if i == active else c["text-secondary"],
                      "label", "middle"))
    # 2. Brightness slider
    ry = row(1, "Yorqinlik")
    tw = 168
    tx = cx + cw - tw
    o.append(rect(tx, ry + 29, tw, 3, c["border-strong"] if hc else c["border-subtle"], 1.5))
    o.append(rect(tx, ry + 29, tw * 0.7, 3, c["accent"], 1.5))
    o.append(f'<circle cx="{tx + tw * 0.7}" cy="{ry + 30.5}" r="8" fill="{c["action-primary"]}"/>')
    # 3. Toggle
    ry = row(2, "Tungi rejim", "Kechqurun ranglar iliqroq")
    kx = cx + cw - 40
    o.append(rect(kx, ry + 20, 40, 22, c["accent"], 11))
    o.append(f'<circle cx="{kx + 29}" cy="{ry + 31}" r="8" fill="{c["text-on-accent"]}"/>')
    # 4. Input with focus ring
    ry = row(3, "Qurilma nomi")
    iw = 188
    ix = cx + cw - iw
    fr = F["border"]["focus-ring-high-contrast" if hc else "focus-ring"]
    off = F["border"]["focus-offset"]
    o.append(rect(ix - off - fr / 2, ry + 12 - off - fr / 2, iw + 2 * off + fr, 34 + 2 * off + fr,
                  "none", R["control"] + off, c["focus-ring"], fr))
    o.append(rect(ix, ry + 12, iw, 34, c["surface-sunken"], R["control"], c["border-strong"], sw))
    o.append(text(ix + 12, ry + 34, "smy-noutbuk", c["text-primary"], "body"))
    o.append(line(ix + 100, ry + 21, ix + 100, ry + 37, c["text-primary"], 1.2))

    # Footer actions, right aligned
    fy = y + wh - 60
    o.append(line(cx, fy - 12, cx + cw, fy - 12, hair, 1))
    o.append(rect(cx + cw - 96, fy, 96, 32, c["action-primary"], R["control"]))
    o.append(text(cx + cw - 48, fy + 21, "Saqlash", c["text-on-action"], "label", "middle"))
    o.append(rect(cx + cw - 206, fy, 100, 32, "none", R["control"], c["border-strong"], sw))
    o.append(text(cx + cw - 156, fy + 21, "Bekor qilish", c["text-primary"], "label", "middle"))

    # Notification: neutral card, colour only in the small status mark
    ny = y + wh + 28
    nw, nh = 320, 64
    nx = x + ww - nw
    shadow = "" if hc else ' filter="url(#elev2)"'
    o.append(rect(nx, ny, nw, nh, c["surface-raised"], R["panel"], hair, sw).replace("/>", shadow + "/>"))
    o.append(rect(nx + 14, ny + 14, 36, 36, c["surface-sunken"], 8, hair, sw))
    o.append(symbol(nx + 22, ny + 22, 20, c["text-primary"], c["accent"]))
    o.append(text(nx + 62, ny + 28, "Yangilanish oʻrnatildi", c["text-primary"], "label"))
    o.append(text(nx + 62, ny + 46, "SMY-OS 0.6 · hozirgina", c["text-secondary"], "caption"))
    o.append(f'<circle cx="{nx + nw - 20}" cy="{ny + 24}" r="3.5" fill="{c["success"]}"/>')

    return "".join(o)


def main(argv):
    themes, data = check_contrast.resolve_themes()
    order = ["dark", "light", "high-contrast"]
    width = COL_W * len(order)
    body = []
    for i, name in enumerate(order):
        body.append(column(i * COL_W, name, data["themes"][name]["label"], themes[name],
                           name == "high-contrast"))
    # Elevation shadows straight from the tokens (one shadow per level).
    defs = "".join(
        f'<filter id="elev{lvl}" x="-20%" y="-20%" width="140%" height="160%">'
        f'<feDropShadow dx="0" dy="{e["y"]}" stdDeviation="{e["blur"] / 2}" '
        f'flood-color="#000000" flood-opacity="{e["opacity"] / 2}"/></filter>'
        for lvl, e in F["elevation"].items() if not lvl.startswith("$") and lvl != "0")
    svg = (f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="660" '
           f'viewBox="0 0 {width} 660"><defs>{defs}</defs>{"".join(body)}</svg>\n')
    pathlib.Path(argv[0]).write_text(svg)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
