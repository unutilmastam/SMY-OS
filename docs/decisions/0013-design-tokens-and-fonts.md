# ADR 0013 — Design tokens: JSON source, generated code not committed; Inter and IBM Plex Mono

- **Status:** Accepted (fonts, colours and the monochrome theme direction approved by the owner, 2026-10-21)
- **Date:** 2026-10-21
- **Phase:** 6

## Context

`docs/DESIGN_PROMPT.md` requires one machine-readable token source with
generated C and CSS, an automated WCAG test, light/dark/high-contrast
themes and an OFL font with checked coverage for Uzbek Latin and Cyrillic.
ROADMAP Phase 6 asks to choose whether generated files are committed.

## Decision

1. **Source of truth:** `design/tokens/*.json` — brand colours, palette
   anchors, generated base ramps, themes (semantic tokens + the list of
   contrast pairs), foundations.
2. **Generated code is not committed.** `smy_tokens.h` and `smy_tokens.css`
   are build products in `build/generated/design/`, regenerated and
   compile-checked on every CI run and uploaded as artefacts. Reasons: no
   drift between source and copy, no review noise, same rule as every other
   build output. Exception: `color-base.json` is committed because people
   read colours from it; `gen_palette.py --check` keeps it identical to its
   source.
3. **Ramps generated in OKLCH** from anchors (brand colours are anchors).
4. **Fonts:** Inter 4.1 (UI) and IBM Plex Mono 2.5.0 (mono), both SIL OFL
   1.1, downloaded from the upstream release archives and pinned by
   SHA-256 (`tools/fetch-fonts.sh`), not committed. Coverage is checked by
   a standard-library `cmap` reader.

## Alternatives considered

- **Commit generated C/CSS and verify identical:** possible, but it doubles
  every token change in review and adds nothing while no component uses
  them yet. Revisit if a consumer outside this build (e.g. a website) needs
  checked-in files.
- **Style Dictionary or a similar token tool:** a Node.js dependency for
  what ~200 lines of Python do; rejected (ADR 0002 keeps tooling to sh and
  Python stdlib).
- **fontTools for coverage:** a pip dependency; the `cmap` subset needed is
  small enough to read directly.
- **Other OFL UI fonts** (Noto Sans, Source Sans 3, Manrope): Noto is wider
  and more neutral in tone; Manrope lacks Cyrillic Uzbek letters in some
  versions; Inter has the best screen hinting at 12–14 px and complete
  coverage. Plex Mono pairs well and covers Uzbek Cyrillic.

## Consequences

- Anyone can regenerate the code with `make design-check`; CI fails on any
  contrast regression, palette drift or missing glyph.
- The light-theme accent `#077479` and the new light status colours were
  approved by the owner on 2026-10-21 (`DESIGN_SYSTEM.md` §3.5).
- Font files enter the ISO only with the desktop (Phase 20); their licence
  texts ship with them.
