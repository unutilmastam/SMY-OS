# SMY-OS Status

Last updated: 2026-10-17 — Phase 0.

Status vocabulary (used exactly): **Implemented · Compiles · Unit-tested ·
Emulator-tested · Hardware-tested · Production-ready.** A passing build is not
proof of correctness or security.

## Summary

There is **no code yet**. The project has its specification, roadmap, root
documentation, five ADRs and an environment report. The SMY Kernel does not
exist yet; nothing boots.

- Installation: **not supported.**
- USB boot: **not supported / not tested.**
- Secure Boot: **not supported.**
- Hardware-tested: **nothing.**

## Phases

| Phase | Status | Notes |
|---|---|---|
| 0 — Foundation | BLOCKED (owner decision: project licence) | Docs, ADRs 0001–0005, environment report and link check done; licence choice pending |
| 1 … 38 | NOT STARTED | see `ROADMAP.md` |

## Components

| Component | Status |
|---|---|
| Documentation skeleton (root docs, ADRs) | Implemented (content), link-checked by `tools/check-links.py` |
| Build system | not started (Phase 1, ADR 0004) |
| SMY Kernel | not started (Phase 2) |
| Design system | not started (Phase 6) |
| Compatibility (C0–C8) | not started (see `COMPATIBILITY.md`) |

## Last check results (Phase 0, cloud session 2026-10-17)

| Check | Command | Result |
|---|---|---|
| Markdown link check | `python3 tools/check-links.py` | PASS locally: 27 files, 0 broken links; negative test (broken file + anchor) correctly fails with exit 1. CI `docs` workflow: PASS (run 39064214535, commit f811b52) |
| Build / unit tests / QEMU | — | not applicable (no code in Phase 0) |

## Known limitations

- Cloud session has no KVM; QEMU will run under TCG (slow) — see
  `docs/architecture/environment.md`.
- Ubuntu's LLVM 18 is used instead of the newest upstream LLVM (ADR 0003).
