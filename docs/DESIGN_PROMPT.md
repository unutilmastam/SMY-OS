# SMY-OS — Design System Prompt (revised v1.1)

> The owner's original design prompt, reviewed and corrected. Corrections are
> listed in `docs/PROMPT_REVIEW.md`. Executed in phases D1–D3 of `ROADMAP.md`;
> desktop components are built only in later phases once display, input and IPC
> exist.

> Brand identity (logo, brand palette, wallpapers, motion and sound
> identity) is specified in `docs/BRAND_PROMPT.md`. Both files describe one
> system: one asset library (`design/`), one token source (`design/tokens/`),
> one icon system.

## Role

You are the principal visual design architect for SMY-OS. Create a completely
original, globally competitive desktop design language. It must not copy
Windows, macOS, GNOME, KDE Plasma or any existing OS interface. The result must
feel like a distinctive new operating system for modern computers, not a
reskinned desktop.

## Requirements

1. **Concept.** Establish an original visual concept with a name and 5–7 written
   design principles, each with a "do / don't" example.
2. **Colour system.** Coordinated palettes for **light, dark and high-contrast**
   modes, built from a small set of base hues with numbered steps (e.g. 0–1000).
   Semantic tokens (`surface`, `text-primary`, `accent`, `danger`, `focus-ring`…)
   map onto base tokens per theme. Every text/background pair used in a
   component must meet **WCAG 2.2 AA** (4.5:1 body text, 3:1 large text and UI
   boundaries); high-contrast mode targets **7:1**. Contrast is checked by an
   automated test, not by eye.
3. **Foundations.** Typography (scale, weights, line heights), spacing scale
   (4 px base), sizing, borders, corner radii, shadows/elevation, motion
   durations and easing, and interaction states (rest, hover, pressed, focus,
   selected, disabled, error) — all as tokens.
4. **Icon system.** Original vector icons with one grid (e.g. 24 × 24 with 2 px
   padding / live area), one stroke width, one corner radius rule, optical
   alignment rules (keylines for circle/square/portrait/landscape), and scaling
   rules (16, 20, 24, 32, 48 px; hinting/snapping for 16 px).
5. **Tokens and components.** One machine-readable source of truth
   (`design/tokens/*.json`) from which code (C headers for the native toolkit,
   CSS for the documentation preview) is **generated** — never hand-copied.
6. **Component designs.** Desktop shell, application launcher, window system
   (frames, title area, controls, resize, snapping, workspaces), file manager,
   settings, notifications, system monitor, application installer. Also define:
   quick settings, search, terminal, lock screen, login, accessibility controls.
7. **Consistency.** All components use only tokens and shared primitives; a
   lint check rejects raw colour/size literals in component specs and code.
8. **Accessibility and displays.** Full keyboard navigation model (focus order,
   shortcuts table, visible focus ring), screen-reader roles/labels, readable
   text sizes (minimum 12 px at 1×, scalable), high-DPI support (scale factors
   1×, 1.25×, 1.5×, 2×; all geometry in logical pixels), reduced-motion setting.
9. **Performance-conscious visuals.** No unnecessary animation, clutter,
   heavy blur or decorative effects. Every effect must work in a **software
   renderer** on the boot framebuffer at 60 Hz; blur is optional and off by
   default. Motion respects "reduce motion".
10. **Separation.** All visual components run in user space, independent of the
    kernel and separate from privileged services. The design library contains
    no kernel code.
11. **Asset library.** All original assets live in `design/` with an index
    (`design/README.md`) listing each asset, author, licence and source file.
12. **No placeholders in the final system.** During development an incomplete
    icon is tracked as `status: draft` in the icon manifest; a release check fails
    if any `draft` icon is used by a component.
13. **Documentation.** Every decision recorded (ADR or `DESIGN_SYSTEM.md`
    rationale section) plus a component implementation plan mapping each
    component to the roadmap phase that builds it.
14. **Incremental implementation and testing.** Build and test step by step:
    tokens → generator → contrast tests → icons + icon lint → rendered
    reference sheets (host-side PNG renders) → native toolkit primitives →
    components inside the real SMY Desktop. No disconnected mockup collections.

## Font licence rule (added)

Typography needs a real font that SMY-OS can redistribute. Options: an
**SIL Open Font License** family (record it in `LICENSES.md`), or a custom font
later. Early kernel/console text uses a small bitmap font that is either
original or OFL/public-domain with recorded licence. Never bundle proprietary
fonts (Segoe UI, San Francisco, etc.).

## Order of work

- **D1 — Design System specification** (documentation + tokens + generator +
  contrast tests). No desktop implementation.
- **D2 — Icon system** (grid, rules, full required icon set as SVG, icon lint,
  rendered reference sheet).
- **D3 — Component specifications** (every component in §6 specified with
  anatomy, states, keyboard behaviour, tokens used; implementation plan).
- Desktop implementation phases come after display, input and IPC (see
  `ROADMAP.md`). Do not implement a complete desktop before then.
