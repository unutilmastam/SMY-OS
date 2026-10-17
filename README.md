# SMY-OS

An original operating system for x86-64 PCs: its own kernel (**SMY Kernel**), its
own desktop (**SMY Desktop**) and design language, and — later — scoped Linux and
Windows application compatibility (**SMY Compatibility**).

SMY-OS is **not** a Linux distribution, a desktop theme or a web app.

**Status:** Phase 2 — the first SMY Kernel boots in QEMU from a hybrid ISO
(BIOS and UEFI, CD and raw disk), prints to the serial log and paints the
screen. It does nothing else yet. Not tested on real hardware; installation
is not supported. See [STATUS.md](STATUS.md) and
[ROADMAP.md](ROADMAP.md).

## How this project is built

The project is developed by Claude Code, phase by phase, driven by the owner
from a phone. GitHub Actions CI is the source of truth for builds and QEMU boot
tests. To advance: open a Claude Code session on this repo and write
**"davom et"**.

## Documentation

| File | Purpose |
|---|---|
| [CLAUDE.md](CLAUDE.md) | Rules loaded automatically in every Claude Code session |
| [ROADMAP.md](ROADMAP.md) | Ordered phases, status, acceptance criteria |
| [STATUS.md](STATUS.md) | What exists and how well it is tested |
| [ARCHITECTURE.md](ARCHITECTURE.md) | Architecture proposal |
| [BUILDING.md](BUILDING.md) | Tools and build commands |
| [TESTING.md](TESTING.md) | Test levels and test matrix |
| [CONTRIBUTING.md](CONTRIBUTING.md) | Code style, warnings, commit rules |
| [SECURITY.md](SECURITY.md) | Security policy and reporting |
| [THREAT_MODEL.md](THREAT_MODEL.md) | Threat model v0 |
| [COMPATIBILITY.md](COMPATIBILITY.md) | Linux/Windows compatibility plan (C0–C8) |
| [DESIGN_SYSTEM.md](DESIGN_SYSTEM.md) | Design system (from Phase 6) |
| [LICENSES.md](LICENSES.md) | Third-party inventory and licence comparison |
| [docs/decisions/](docs/decisions/README.md) | Architecture Decision Records |
| [docs/architecture/environment.md](docs/architecture/environment.md) | Build environment report |
| [docs/architecture/hardware.md](docs/architecture/hardware.md) | Hardware assumptions |
| [docs/install/iso-spec.md](docs/install/iso-spec.md) | ISO build specification and boot path |
| [docs/MASTER_PROMPT.md](docs/MASTER_PROMPT.md) | Full project specification |
| [docs/DESIGN_PROMPT.md](docs/DESIGN_PROMPT.md) | Visual design specification |
| [docs/INSTALLER_PROMPT.md](docs/INSTALLER_PROMPT.md) | ISO, live USB and installer specification |
| [docs/PROMPTS.md](docs/PROMPTS.md) | Prompt applied to each phase |
| [docs/PROMPT_REVIEW.md](docs/PROMPT_REVIEW.md) | Review of the original prompts (Uzbek) |

## License

Apache License 2.0 — see [LICENSE](LICENSE), [NOTICE](NOTICE) and
[LICENSES.md](LICENSES.md) for third-party components.
