# SMY-OS

An original operating system for x86-64 PCs: its own kernel (**SMY Kernel**), its
own desktop (**SMY Desktop**) and design language, and — later — scoped Linux and
Windows application compatibility (**SMY Compatibility**).

**Status:** planning. No code yet. See `ROADMAP.md` (Phase 0 is next) and, once
created, `STATUS.md`.

## How this project is built

The project is developed by Claude Code, phase by phase, driven by the owner
from a phone. GitHub Actions CI is the source of truth for builds and QEMU boot
tests.

| File | Purpose |
|---|---|
| `CLAUDE.md` | Rules loaded automatically in every Claude Code session |
| `docs/MASTER_PROMPT.md` | Full project specification (revised) |
| `docs/DESIGN_PROMPT.md` | Visual design specification (revised) |
| `docs/INSTALLER_PROMPT.md` | ISO, live USB and installer specification (revised) |
| `ROADMAP.md` | Ordered phases, status, acceptance criteria |
| `docs/PROMPTS.md` | Prompt applied to each phase |
| `docs/PROMPT_REVIEW.md` | Review of the original prompts (Uzbek) |

To advance: open a Claude Code session on this repo and write **"davom et"**.

## License

All rights reserved — license pending (owner decision in Phase 0).
