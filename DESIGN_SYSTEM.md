# SMY Design System

Status: **not started.** The design system is specified in Phase 6 (D1),
icons in Phase 12 (D2) and component specifications in Phase 17 (D3) of
`ROADMAP.md`. The binding requirements are in `docs/DESIGN_PROMPT.md`.

## What will be here (from Phase 6)

- Visual concept name and 5–7 design principles with do/don't examples.
- Colour system: base hues with numbered steps, semantic tokens, light / dark /
  high-contrast themes.
- Foundations: typography, 4 px spacing scale, sizing, borders, radii,
  elevation, motion, interaction states.
- Token source of truth: `design/tokens/*.json`, with generated C headers and
  CSS (never hand-copied).
- Accessibility targets, checked automatically in CI: **WCAG 2.2 AA**
  (4.5:1 body text, 3:1 large text and UI boundaries); high-contrast theme
  **7:1**.
- Rationale for every decision.

## Fixed constraints (already decided)

- Original design: no copy of Windows, macOS, GNOME, KDE or other interfaces.
- Every effect must run in a software renderer on the boot framebuffer at
  60 Hz; blur off by default; "reduce motion" respected.
- Fonts: SIL Open Font License or original only, recorded in `LICENSES.md`.
- The desktop is implemented only after display, input and IPC exist
  (Phase 18+).
