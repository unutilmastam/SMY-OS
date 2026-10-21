#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 The SMY-OS Authors
"""Check a QEMU screendump of the SMY-OS boot screen.

Language: Python 3, standard library only (ADR 0002).

Usage: tests/integration/check_boot_screen.py <screen.ppm>

Pass when:
  - the four corners and the screen edges are VOID (#080B12),
  - the centred logo is present: the centre box contains WHITE (#F4F7FF)
    stroke pixels and CYAN (#54F6E8) core pixels, the core up and to the
    right of the centre (design/brand/logos/smy-boot-logo.svg),
  - nothing but VOID lies outside the logo box.
Colours are compared with a small tolerance for framebuffer channel sizes.
"""

import sys

VOID = (0x08, 0x0B, 0x12)
WHITE = (0xF4, 0xF7, 0xFF)
CYAN = (0x54, 0xF6, 0xE8)
TOL = 8


def read_ppm(path):
    with open(path, "rb") as fh:
        data = fh.read()
    fields, pos = [], 0
    while len(fields) < 4:
        while data[pos:pos + 1].isspace():
            pos += 1
        if data[pos:pos + 1] == b"#":
            pos = data.index(b"\n", pos) + 1
            continue
        end = pos
        while not data[end:end + 1].isspace():
            end += 1
        fields.append(data[pos:end])
        pos = end
    pos += 1
    if fields[0] != b"P6" or int(fields[3]) != 255:
        raise SystemExit(f"check_boot_screen: {path}: not an 8-bit binary PPM")
    w, h = int(fields[1]), int(fields[2])
    if len(data) - pos < w * h * 3:
        raise SystemExit(f"check_boot_screen: {path}: truncated")
    return w, h, data[pos:pos + w * h * 3]


def near(px, colour):
    return all(abs(a - b) <= TOL for a, b in zip(px, colour))


def main(argv):
    if len(argv) != 1:
        print(__doc__, file=sys.stderr)
        return 2
    w, h, pix = read_ppm(argv[0])

    def at(x, y):
        i = 3 * (y * w + x)
        return tuple(pix[i:i + 3])

    errors = []
    for x, y in ((0, 0), (w - 1, 0), (0, h - 1), (w - 1, h - 1), (w // 2, 0), (0, h // 2)):
        if not near(at(x, y), VOID):
            errors.append(f"pixel ({x},{y}) is {at(x, y)}, expected VOID {VOID}")

    # The logo is at most a fifth of the shorter side, centred.
    half = min(w, h) // 10 + 1
    cx, cy = w // 2, h // 2
    white = cyan = cyan_ne = stray = 0
    for y in range(h):
        for x in range(w):
            px = at(x, y)
            inside = abs(x - cx) <= half and abs(y - cy) <= half
            if not inside:
                if not near(px, VOID):
                    stray += 1
                continue
            if near(px, WHITE):
                white += 1
            elif near(px, CYAN):
                cyan += 1
                if x > cx and y < cy:
                    cyan_ne += 1
    if white < 50:
        errors.append(f"logo stroke missing: {white} WHITE pixels in the centre box")
    if cyan < 20:
        errors.append(f"logo core missing: {cyan} CYAN pixels in the centre box")
    elif cyan_ne < cyan * 9 // 10:
        errors.append(f"logo core misplaced: {cyan_ne} of {cyan} CYAN pixels top-right")
    if stray:
        errors.append(f"{stray} non-VOID pixels outside the logo box")

    if errors:
        for e in errors:
            print(f"check_boot_screen: FAIL {argv[0]}: {e}")
        return 1
    print(f"check_boot_screen: PASS {argv[0]} ({w}x{h}, {white} white, {cyan} cyan)")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
