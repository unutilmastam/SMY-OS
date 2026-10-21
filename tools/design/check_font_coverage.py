#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 The SMY-OS Authors
"""Check that fonts contain every character SMY-OS's interface languages need.

Language: Python 3, standard library only (ADR 0002). Reads the font's
`cmap` table directly (TrueType/OpenType, formats 4 and 12), so no font
library is needed.

Required set (docs/BRAND_PROMPT.md §10, DESIGN_SYSTEM.md "Typography"):
  - printable ASCII;
  - Uzbek Latin: Oʻ oʻ Gʻ gʻ use U+02BB MODIFIER LETTER TURNED COMMA, plus
    the typographic apostrophe U+2019 (tutuq belgisi) and quotes;
  - Uzbek Cyrillic: the full Russian alphabet plus Ўў Ққ Ғғ Ҳҳ;
  - common punctuation and symbols used by the UI (« » – — … • € № ©).

Usage: tools/design/check_font_coverage.py <font.ttf|otf>...
"""

import struct
import sys

REQUIRED = {
    "printable ASCII": [chr(c) for c in range(0x20, 0x7F)],
    "Uzbek Latin": list("OʻoʻGʻgʻ’‘"),
    "Cyrillic (Russian)": [chr(c) for c in range(0x0410, 0x0450)] + ["Ё", "ё"],
    "Uzbek Cyrillic": list("ЎўҚқҒғҲҳ"),
    "punctuation and symbols": list("«»–—…•€№©“”"),
}


def read_cmap(path):
    with open(path, "rb") as fh:
        data = fh.read()
    if data[:4] == b"ttcf":
        raise SystemExit(f"{path}: font collections are not supported")
    num_tables = struct.unpack_from(">H", data, 4)[0]
    cmap_off = None
    for i in range(num_tables):
        tag, _, offset, _ = struct.unpack_from(">4sIII", data, 12 + 16 * i)
        if tag == b"cmap":
            cmap_off = offset
    if cmap_off is None:
        raise SystemExit(f"{path}: no cmap table")
    _, n_sub = struct.unpack_from(">HH", data, cmap_off)
    chosen = None
    for i in range(n_sub):
        platform, encoding, offset = struct.unpack_from(">HHI", data, cmap_off + 4 + 8 * i)
        fmt = struct.unpack_from(">H", data, cmap_off + offset)[0]
        # Prefer the full-Unicode table (format 12), else the BMP one (format 4).
        if (platform, encoding) in ((3, 10), (0, 4), (0, 6)) and fmt == 12:
            chosen = (12, cmap_off + offset)
            break
        if (platform, encoding) in ((3, 1), (0, 3)) and fmt == 4 and chosen is None:
            chosen = (4, cmap_off + offset)
    if chosen is None:
        raise SystemExit(f"{path}: no Unicode cmap subtable (format 4 or 12)")
    fmt, off = chosen
    codepoints = set()
    if fmt == 12:
        n_groups = struct.unpack_from(">I", data, off + 12)[0]
        for g in range(n_groups):
            start, end, glyph = struct.unpack_from(">III", data, off + 16 + 12 * g)
            for c in range(start, end + 1):
                if glyph + (c - start) != 0:
                    codepoints.add(c)
    else:
        seg_x2 = struct.unpack_from(">H", data, off + 6)[0]
        segs = seg_x2 // 2
        ends = struct.unpack_from(f">{segs}H", data, off + 14)
        starts = struct.unpack_from(f">{segs}H", data, off + 16 + seg_x2)
        deltas = struct.unpack_from(f">{segs}h", data, off + 16 + 2 * seg_x2)
        range_base = off + 16 + 3 * seg_x2
        ranges = struct.unpack_from(f">{segs}H", data, range_base)
        for s in range(segs):
            for c in range(starts[s], ends[s] + 1):
                if c == 0xFFFF:
                    continue
                if ranges[s] == 0:
                    glyph = (c + deltas[s]) & 0xFFFF
                else:
                    addr = range_base + 2 * s + ranges[s] + 2 * (c - starts[s])
                    glyph = struct.unpack_from(">H", data, addr)[0]
                    if glyph != 0:
                        glyph = (glyph + deltas[s]) & 0xFFFF
                if glyph != 0:
                    codepoints.add(c)
    return codepoints


def main(argv):
    if not argv:
        print(__doc__, file=sys.stderr)
        return 2
    failed = False
    for path in argv:
        have = read_cmap(path)
        missing = []
        for group, chars in REQUIRED.items():
            for ch in sorted(set(chars)):
                if ord(ch) not in have:
                    missing.append(f"{group}: U+{ord(ch):04X} {ch}")
        name = path.rsplit("/", 1)[-1]
        if missing:
            failed = True
            print(f"check_font_coverage: FAIL {name}: {len(missing)} missing")
            for m in missing:
                print(f"  {m}")
        else:
            total = len(set().union(*REQUIRED.values()))
            print(f"check_font_coverage: OK {name} ({len(have)} characters; all {total} required present)")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
