# SMY-OS — Brand Identity Prompt: "SMY Quantum Minimalism" (revised v1.1)

> The owner's brand identity prompt, reviewed and corrected. Corrections are
> listed in `docs/PROMPT_REVIEW.md` (section "Brand prompt"). Executed in the
> phases of `ROADMAP.md` marked track **Brand** (6B, 12, 12B and the brand
> parts of 30, 33). Where this file and the original disagree, this file wins;
> where it and `docs/DESIGN_PROMPT.md` overlap, they are one system (see §0).

## 0. How this fits the existing design work (added)

- **One asset library.** All visual assets live under `design/`
  (`docs/DESIGN_PROMPT.md` §11). Brand assets go to `design/brand/`, icons
  to `design/icons/`, wallpapers to `design/wallpapers/`. There is no
  separate top-level `branding/` tree.
- **One token source of truth.** Colours, type, spacing and motion are
  defined once in `design/tokens/*.json`; CSS, C headers and SVG colour
  references are **generated** from them (`docs/DESIGN_PROMPT.md` §5). The
  brand palette below becomes the *base* layer of those tokens; UI themes map
  semantic tokens onto it.
- **Brand ≠ UI theme.** The brand palette expresses identity (logo,
  wallpapers, marketing). UI themes (light, dark, **high contrast**) must also
  pass WCAG 2.2 AA (and 7:1 for high contrast) by automated test.
- **Order.** The identity (logo, palette) is decided before the UI design
  system is finalised, because UI tokens derive from it. Kernel work continues
  in between; the desktop is built only after display, input and IPC exist.

## 1. Role and vision

Act as senior brand identity designer, design-systems architect, vector
graphics specialist, UI/UX designer and asset engineer. Create **real files**,
validate them, and report only what exists.

SMY-OS should communicate: technological independence, precision,
innovation, reliability, minimalism, sophistication, performance,
adaptability. It must read as a serious long-term technology platform — not
a gaming logo, cryptocurrency project, generic AI startup or decorative
concept.

Design language name: **SMY Quantum Minimalism** — an internal project name,
not a registered trademark. Geometric precision, controlled negative space,
restrained luminous accents, consistent proportions, no unnecessary effects.

## 2. Originality

- Do not copy or closely imitate Apple, Microsoft/Windows, Linux
  distributions, Android, Google, other technology logos, existing OS icon
  sets, commercial icon packs, copyrighted wallpapers or proprietary design
  systems. Do not recolour an existing logo. No "letter in a gradient
  circle".
- S, M, Y are optional inspiration, not a requirement.
- Explore: interconnected geometric layers, a central computational core,
  modular architecture, an abstract digital gateway, controlled negative
  space, a continuous or segmented geometric path.
- Produce **at least three** distinct concepts as editable SVG and evaluate
  them on: recognition at 16 px, simplicity, memorability, scalability,
  monochrome performance, OS suitability, distinctiveness from well-known
  marks, fit with the whole system. Record the evaluation as a table.
- **OWNER DECISION (added):** the final choice of concept belongs to the
  owner. Claude prepares the three concepts with a preview sheet (dark,
  light, monochrome, 16/32/256 px) and a recommendation; the owner picks.
- Never claim worldwide uniqueness or legal clearance. A visual similarity
  check against well-known OS marks is documented as a manual review, not
  as a legal search.

## 3. Repository rules

- Inspect the repository first; extend existing conventions
  (`design/`, `design/tokens/`, `tools/design/`, `docs/`).
- Never touch kernel, bootloader or build code except where an asset is
  integrated on purpose (boot logo, ISO), and then through the normal phase
  workflow with tests.
- Brand assets must be usable without the OS (plain SVG/PNG + docs).

## 4. Colour specification (initial, owner-provided)

Dark/brand palette:

| Token | Value |
|---|---|
| `smy.void` | `#080B12` |
| `smy.deep` | `#101522` |
| `smy.surface` | `#171E2D` |
| `smy.cyan` | `#54F6E8` |
| `smy.pulse` | `#7B72FF` |
| `smy.ice` | `#D8FFFC` |
| `smy.white` | `#F4F7FF` |
| `smy.muted` | `#929BAF` |
| `smy.graphite` | `#293247` |

Semantic (dark): success `#5DE6A3`, warning `#FFD166`, error `#FF647C`,
info `#65BFFF`.

Light: background `#F5F7FB`, surface `#FFFFFF`, elevated `#EAF0F7`, text
`#141A29`, muted `#5D687C`, border `#DCE3ED`, accent `#087F83`, violet
`#6558DB`.

Measured contrast (WCAG 2.x formula, computed 2026-10-18) and the resulting
rules — **added**:

| Pair | Ratio | Rule |
|---|---|---|
| white on void | 18.4 | body text ✓ |
| muted on void / on surface | 7.1 / 6.0 | secondary text ✓ |
| cyan on void | 14.7 | accent, text ✓ |
| pulse on void / on surface | 5.3 / 4.5 | text only at ≥ 4.5 ✓ (borderline on surface) |
| graphite on void | 1.5 | decoration only — **not** a control boundary (needs 3:1) |
| cyan on light background | 1.25 | **never** use cyan on light backgrounds; use `#087F83` |
| light accent on white / on light background | 4.8 / **4.48** | **fails 4.5 on `#F5F7FB`** — darken (proposal: `#077479`: 5.2 on `#F5F7FB`, 4.8 on elevated `#EAF0F7`, 5.6 on white) or use only on white |
| light muted on light background | 5.2 | ✓ |
| light border on white | 1.3 | decoration only; focus rings and control outlines need ≥ 3:1 |
| success / warning / info on light background | 1.5 / 1.3 / 1.9 | **fail** — light-theme semantic colours are **missing** and must be defined |
| void on cyan (text on accent button) | 14.7 | ✓ |

These numbers are reproduced by the automated contrast test of Phase 6
(`docs/DESIGN_PROMPT.md` §2). Any change to an owner-provided value is
proposed, documented and approved by the owner, never applied silently. A
**high-contrast theme** is also required (not in the original palette).

Hierarchy:

- Primary logo: symbol + wordmark + restrained cyan accent.
- Dark-background and light-background logos; monochrome (one solid
  colour); micro mark (simplified silhouette); special-edition artwork only as
  a separate, labelled variant.
- The primary logo must work with **no** gradient, shadow, glow,
  transparency or animation.

## 5. Logo family

All variants derive from one approved master symbol:

A master logo (symbol + wordmark) · B symbol · C wordmark, spelled exactly
**SMY-OS** (hyphen kept) · D app icon · E boot logo · F installer logo ·
G monochrome black/white · H light · I dark · J micro mark · K developer mark ·
L community mark (only if needed).

Required SVG layouts: symbol only, horizontal logo, compact (stacked) logo,
monochrome, small-size icon.

## 6. Vector construction

- Master format SVG; valid XML; sensible `viewBox`; transparent background;
  no filters, no embedded raster, no external references, no `<text>` in
  final logos (the wordmark is drawn as paths).
- **Wordmark letters are drawn as original geometry (added).** Converting a
  font's outlines into the wordmark is avoided, so the logo carries no font
  licence question.
- Editable sources are kept; exports are generated, not hand-made.
- Raster exports (PNG at 16, 24, 32, 48, 64, 128, 256, 512, 1024 px) are
  **generated by a build step** (`make brand`) into `build/brand/` and
  published as CI artefacts — consistent with the rule that generated files
  stay out of git. Only SVG sources are committed (documented in
  `design/brand/README.md`).

## 7. Proportions and safe area

Document: construction grid, outer proportions, optical centre, minimum
clear space, symbol-to-wordmark ratio, wordmark spacing, minimum sizes,
approved backgrounds, misuse examples. Test sizes 16–1024 px; below 32 px
use the micro mark instead of shrinking detail.

## 8. Iconography

Specification (grid, stroke, radii, padding, optical alignment,
filled/outlined, monochrome, accent rules, small-size simplification,
light/dark) — merged with `docs/DESIGN_PROMPT.md` §4 (one icon system, not
two).

Categories (in addition to the DESIGN_PROMPT list):

- System: overview, settings, updates, power, restart, shutdown, device info
- Files: folder, file, image, video, audio, archive, download, upload,
  search, trash, storage
- Connectivity: network, Wi-Fi, Ethernet, Bluetooth, connected device,
  offline
- Security: privacy, permissions, firewall, lock, secure storage, user
  account
- Development: terminal, code editor, build tools, debugging, processes,
  system monitor
- Media: music, video, camera, images, creative studio
- Applications: launcher, browser, calculator, calendar, notes, help

Naming: `smy-icon-<name>.svg` (lower-case, hyphens). Every listed icon must
be a real drawing; the manifest marks `draft` / `final`.

## 9. Wallpapers

At least: primary dark, secondary dark, light, minimal low-distraction, and a
high-resolution promotional wallpaper. Procedural SVG or programmatic
generation (source kept in `design/wallpapers/source/`), exports generated.
Readability rule: desktop icons and window text placed on the wallpaper must
still meet the UI contrast requirements; test with a reference overlay.
Wallpapers are shipped in the ISO only when the desktop exists (Phase 20).

## 10. Typography

Roles: brand wordmark (custom paths, §6), headings, desktop UI, app names,
technical docs, terminal.

Candidate fonts: **Inter** (UI) and **IBM Plex Mono** (technical/terminal),
both published under the SIL Open Font License — **verify the licence text
and version of the exact files** before adding them, record them in
`LICENSES.md`, and never call them SMY typefaces. Check glyph coverage for
the interface languages, including **Uzbek Latin `Oʻ oʻ Gʻ gʻ` (U+02BB) and
Cyrillic `Ўў Ққ Ғғ Ҳҳ`** — added; the check is an automated script, not an
assumption. The kernel's early text console keeps its own small bitmap font
(`docs/DESIGN_PROMPT.md`, font rule).

## 11. Motion identity (specification)

Timing, easing, direction and reduced-motion behaviour for boot, logo
transitions, app launch, windows, notifications, installer. Subtle, short,
optional; the logo is recognisable without animation. Every motion must run
in the software renderer at 60 Hz (`docs/DESIGN_PROMPT.md` §9). No animated
asset is claimed until it exists and is rendered.

## 12. Sound identity (specification only for now)

Startup, shutdown, notification, error, confirmation: short, restrained,
original — never copied from another OS. SMY-OS has no audio drivers yet;
sound assets are **SPECIFIED BUT NOT IMPLEMENTED** until an audio phase
exists (defined in Phase 38 planning). No placeholder audio files.

## 13. Asset directory (adapted to this repository)

```
design/
  README.md                 asset index (author, licence, source per asset)
  tokens/                   single source of truth (colours, type, spacing, motion)
  brand/
    BRAND_GUIDELINES.md
    BRAND_ASSET_MANIFEST.md
    concepts/               concept-a.svg, concept-b.svg, concept-c.svg
    logos/                  master, symbol, wordmark, app-icon, mono,
                            light, dark, micro, boot, installer, developer
    motion/  sound/         specifications
  icons/<category>/smy-icon-*.svg
  wallpapers/source/        procedural sources
tools/design/               generators, validators, preview-sheet renderer
build/brand/                generated PNG exports and preview sheets (not in git)
```

Create directories only when they receive real files.

## 14. Validation (automated, in CI)

1. Every manifest entry exists and every asset file is in the manifest.
2. SVGs parse; `viewBox` valid; no external references, scripts, `<image>`,
   filters or `<text>` in logos.
3. Wordmark spelled `SMY-OS` (checked via a recorded path signature or
   metadata, plus the visual preview).
4. Naming convention.
5. Raster exports generated at the expected sizes.
6. Contrast of every documented colour pair (light, dark, high contrast).
7. Micro mark rendered at 16 px for the preview sheet.
8. Licences recorded for every third-party file.

Preview sheets are generated for human review. A visual check is reported as
done only if someone (Claude via the rendered image, or the owner) actually
looked at it.

## 15. Trademark and licensing

Provenance for every external resource (name, source, licence, version,
redistribution terms, modifications) in `LICENSES.md`. No claims of
registration, global uniqueness or freedom from conflicts. No domain
purchase, trademark filing or any expense — those are owner decisions
outside the code.

## 16. Quality review

Before a brand phase is VERIFIED: coherence, distinctiveness, simplicity,
scalability, contrast, readability, vector quality, file integrity, naming,
licensing, compatibility with OS components — plus a written list of
remaining weaknesses. Weak assets are refined, not declared finished.

## 17. Milestones → roadmap phases (adapted)

| Original milestone | Phase |
|---|---|
| 1 inspect + plan, 2 three concepts, 3 evaluation | **6B-1** (ends with OWNER DECISION) |
| 4 master logo family, 5 tokens/type/proportions, 8 boot + installer logo files | **6B-2** (after the owner's choice) + Phase 6 (tokens) |
| 6 icons | Phase 12 (D2) |
| 7 wallpapers | **12B** |
| boot logo shown by the kernel | 6B-2 (rendered by the kernel on the boot framebuffer, replacing the interim square) |
| installer logo in the installer UI | Phase 33 |
| 9 validation, 10 documentation | every brand phase |

The original instruction "do all milestones without stopping" is replaced by
the project rule: **one phase per owner command**.

## 18. Status vocabulary for brand assets

IMPLEMENTED AND VERIFIED · IMPLEMENTED BUT NOT FULLY VERIFIED · SPECIFIED
BUT NOT IMPLEMENTED · BLOCKED BY MISSING TOOLS OR DEPENDENCIES. Never report
a planned asset as an existing file.
