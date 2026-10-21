# SMY-OS brand guidelines — logo

The SMY-OS logo ("Module", ADR 0011) and how to use it. Source of truth for
the geometry: `tools/design/brand_logo.py`; colours:
`design/tokens/brand-colors.json`; files: `BRAND_ASSET_MANIFEST.md`.
Specification: `docs/BRAND_PROMPT.md`. No trademark search or registration
has been done; nothing here claims legal clearance.

## 1. Idea

A rounded-square frame — the system — whose top-right corner is completed by
a separate, solid core module. The open corner and the detached core say
"modular, built from its own parts". One accent colour, flat shapes, no
gradient, shadow, glow or transparency.

## 2. Construction grid (symbol)

A 100 × 100 unit grid with an 8-unit margin (live area 8..92). One ratio
system from the stroke width S:

| Element | Value |
|---|---|
| Frame stroke S | 14 |
| Frame corner radius (centre line) R | 20 |
| Core square | 2 S = 28 (corner radius 6), aligned to the top-right live corner |
| Gap between frame ends and core | S / 2 = 7 |
| Joins | round |

The frame's two ends stop exactly one gap before the core, horizontally and
vertically, so the open corner reads as deliberate. Optical centre: the
core adds weight top-right; the symbol is centred geometrically, not
optically, because the frame dominates.

**Micro mark** (16–24 px): S = 18, R = 14; core and gap follow the same rules.
At these sizes the master's 7-unit gap would close under pixel snapping.

**Developer mark**: the core drawn as an outline (S / 2), not filled.

## 3. Wordmark

`SMY-OS`, always upper case with the hyphen. Original monoline letters drawn
as paths (no font outlines, so no font licence question): cap height 80,
stroke 12, radius 17, tracking 14 on the wordmark grid
(`tools/design/brand_concepts.py`). Labels (`INSTALLER`, `DEVELOPER`) use the
same letter system with wide tracking (40) in a muted colour.

## 4. Lockups and proportions

| Lockup | Proportion (symbol = 100) |
|---|---|
| Horizontal (master) | wordmark cap height 46, gap symbol→wordmark 34, wordmark vertically centred on the symbol |
| Horizontal with label | wordmark raised; label cap height 15 aligned to the symbol baseline |
| Stacked | wordmark cap height 30, 26 below the symbol, centred |
| App icon | symbol at 62 % of a rounded tile (corner radius 22 %) |

## 5. Clear space and minimum sizes

- Clear space on every side: **2 S = 28 units** (28 % of the symbol height).
  It is built into every lockup file's canvas; nothing may enter it.
- Minimum sizes: symbol **32 px**; below that use the micro mark
  (16–24 px). Horizontal logo: **32 px** symbol height (text stays legible
  at the 46 % cap height ≈ 15 px). Stacked logo: 48 px symbol height.

## 6. Colours and approved backgrounds

| Use | Frame + wordmark | Core | Background |
|---|---|---|---|
| Dark (primary) | `smy.white` `#F4F7FF` | `smy.cyan` `#54F6E8` | `smy.void` `#080B12` or other brand darks (`deep`, `surface`) |
| Light | `light.text` `#141A29` | `light.accent` `#087F83` | `light.background` `#F5F7FB`, white |
| Monochrome | one colour (black or white) | same colour | any background with ≥ 3:1 contrast |

Measured contrast (WCAG 2.x; checked by `validate_brand.py`): white on void
18.4, cyan on void 14.7, light text on light background 16.2, light accent on
light background 4.48 (passes the 3:1 rule for graphics; the core is a
graphic, not text). Cyan is **never** used on light backgrounds (1.25:1).

Open owner decision (Phase 6): `#087F83` fails 4.5:1 for *text* on
`#F5F7FB`; proposal `#077479`. The logo is unaffected either way.

## 7. Misuse

Do not:

1. recolour the core in anything but the accent of its variant, or put the
   accent on the frame;
2. close the open corner or attach the core to the frame;
3. stretch, skew, rotate or change the frame/core proportions;
4. add gradients, shadows, glows, outlines or transparency;
5. place the dark-background logo on light backgrounds (or vice versa), or
   on busy photos without a solid plate;
6. set `SMY-OS` in a font instead of the drawn wordmark, change the case or
   drop the hyphen;
7. shrink the master symbol below 32 px instead of using the micro mark;
8. animate the primary logo (motion is a separate specification,
   `docs/BRAND_PROMPT.md` §11).

## 8. Boot screen

The kernel shows `smy-boot-logo.svg` centred on `smy.void`. The SVG is
rendered at build time to 96/128/192/256 px run-length-encoded pixels
(`tools/design/boot_logo.py`); the kernel picks the largest size ≤ one fifth
of the shorter screen side. Verified by a QEMU screendump pixel test on BIOS
and UEFI (`make boot-screen`, `TESTING.md`).

## 9. Remaining weaknesses (honest list)

- The solid core can still read as a "notification badge" at a glance;
  the 7-unit gap and 2 S core are the best balance found so far.
- The micro mark at 16 px depends on pixel snapping; it is legible but soft.
- The wordmark is a monoline geometric; its `M` and `Y` are close to many
  geometric sans letterforms — distinctiveness comes from the symbol.
