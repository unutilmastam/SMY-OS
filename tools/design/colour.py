# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 The SMY-OS Authors
"""Colour maths shared by the design tools: sRGB <-> OKLab/OKLCH and the
WCAG 2.x contrast ratio. Python 3 standard library only (ADR 0002).

OKLab: Björn Ottosson, "A perceptual color space for image processing"
(2020), published matrices. WCAG: relative luminance and contrast ratio as
defined in WCAG 2.2, 1.4.3.
"""

import math


def hex_to_rgb(h):
    h = h.lstrip("#")
    if len(h) != 6:
        raise ValueError(f"bad colour #{h}")
    return tuple(int(h[i:i + 2], 16) / 255 for i in (0, 2, 4))


def rgb_to_hex(rgb):
    return "#" + "".join(f"{round(min(max(c, 0.0), 1.0) * 255):02X}" for c in rgb)


def _to_linear(c):
    return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4


def _to_gamma(c):
    return 12.92 * c if c <= 0.0031308 else 1.055 * c ** (1 / 2.4) - 0.055


def rgb_to_oklab(rgb):
    r, g, b = (_to_linear(c) for c in rgb)
    l_ = 0.4122214708 * r + 0.5363325363 * g + 0.0514459929 * b
    m_ = 0.2119034982 * r + 0.6806995451 * g + 0.1073969566 * b
    s_ = 0.0883024619 * r + 0.2817188376 * g + 0.6299787005 * b
    l_, m_, s_ = (math.copysign(abs(v) ** (1 / 3), v) for v in (l_, m_, s_))
    return (0.2104542553 * l_ + 0.7936177850 * m_ - 0.0040720468 * s_,
            1.9779984951 * l_ - 2.4285922050 * m_ + 0.4505937099 * s_,
            0.0259040371 * l_ + 0.7827717662 * m_ - 0.8086757660 * s_)


def oklab_to_rgb_linear(lab):
    L, a, b = lab
    l_ = (L + 0.3963377774 * a + 0.2158037573 * b) ** 3
    m_ = (L - 0.1055613458 * a - 0.0638541728 * b) ** 3
    s_ = (L - 0.0894841775 * a - 1.2914855480 * b) ** 3
    return (4.0767416621 * l_ - 3.3077115913 * m_ + 0.2309699292 * s_,
            -1.2684380046 * l_ + 2.6097574011 * m_ - 0.3413193965 * s_,
            -0.0041960863 * l_ - 0.7034186147 * m_ + 1.7076147010 * s_)


def oklch_to_rgb(L, C, H):
    """OKLCH -> sRGB, reducing chroma until the colour fits the gamut."""
    lo, hi = 0.0, C
    for _ in range(40):
        lin = oklab_to_rgb_linear((L, hi * math.cos(H), hi * math.sin(H)))
        if all(-1e-6 <= c <= 1 + 1e-6 for c in lin):
            break
        mid = (lo + hi) / 2
        lin = oklab_to_rgb_linear((L, mid * math.cos(H), mid * math.sin(H)))
        if all(-1e-6 <= c <= 1 + 1e-6 for c in lin):
            lo = mid
        else:
            hi = mid
    else:
        lin = oklab_to_rgb_linear((L, lo * math.cos(H), lo * math.sin(H)))
    return tuple(_to_gamma(min(max(c, 0.0), 1.0)) for c in lin)


def rgb_to_oklch(rgb):
    L, a, b = rgb_to_oklab(rgb)
    return L, math.hypot(a, b), math.atan2(b, a)


def luminance(hex_colour):
    r, g, b = (_to_linear(c) for c in hex_to_rgb(hex_colour))
    return 0.2126 * r + 0.7152 * g + 0.0722 * b


def contrast(fg, bg):
    la, lb = sorted((luminance(fg), luminance(bg)), reverse=True)
    return (la + 0.05) / (lb + 0.05)
