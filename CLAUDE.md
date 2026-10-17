# CLAUDE.md — SMY-OS

This file is loaded automatically in every Claude Code session. Keep it short.
The authoritative instructions are:

- `docs/MASTER_PROMPT.md` — the full project specification (kernel, compatibility,
  security, graphics, workflow). Read it before any change.
- `docs/DESIGN_PROMPT.md` — the visual design specification (design system, icons,
  desktop components).
- `docs/INSTALLER_PROMPT.md` — ISO, live USB, installer and deployment
  specification.
- `docs/BRAND_PROMPT.md` — brand identity "SMY Quantum Minimalism" (logo,
  palette, icons, wallpapers, typography, motion, sound).
- `ROADMAP.md` — the ordered list of phases, their status and acceptance criteria.
- `docs/PROMPTS.md` — the prompt to execute for each phase.

## What SMY-OS is

An original operating system for x86-64 PCs: own kernel (SMY Kernel), own system
services, own desktop (SMY Desktop), own design language, and — later — carefully
scoped Linux and Windows application compatibility. It is NOT a Linux
distribution, a desktop theme or a web app.

## Owner's environment (important)

- The owner has **no computer**. He works from a phone/iPad and drives Claude Code
  in the cloud against this GitHub repo.
- Therefore **GitHub Actions CI is the source of truth** for build, tests and the
  QEMU boot test. Every phase must keep CI green.
- Cloud sessions are ephemeral. Install missing tools (e.g. `qemu-system-x86`,
  `xorriso`) with `apt-get` inside the session when possible. If a tool cannot be
  installed, say exactly which checks only ran in CI. Never fake results.
- The owner cannot test on real hardware. Everything is emulator-first.
- The owner does have a Windows 11 laptop with VirtualBox and can run an ISO
  there by hand when asked (confirmed 2026-10-18). Development itself still
  happens in cloud sessions; never ask the owner to build or debug locally.

## Autopilot (owner does not copy prompts)

When the owner writes a short command such as **"davom et"**, **"boshla"**,
**"keyingi"**, "continue" or "next":

1. Open `ROADMAP.md` and find the first phase whose status is not `VERIFIED`.
2. If that phase is `IN PROGRESS` or `BLOCKED`, finish or unblock it first.
3. Otherwise take the **Standard phase prompt** from `docs/PROMPTS.md`, apply it to
   that phase's section in `ROADMAP.md`, and treat it as the owner's full
   instruction.
4. Do not wait for plan approval — state the plan briefly in the report and
   proceed — unless the phase is marked **OWNER DECISION** or the action is
   irreversible / external (publishing, purchases, license choice).
5. Finish the whole phase: implement, build, run all tests, fix, update docs,
   `ROADMAP.md` and `STATUS.md`, commit and push to `main`.
6. Report in **Uzbek (Latin)**, short. End with one line:
   `Keyingi: Phase N — "davom et" deb yozing.`
7. Stop. **One phase per command.** Never run two phases in one go.

If the owner writes **"xato"** / **"tuzat"** with a description or CI error, use
the "Error fix" prompt from `docs/PROMPTS.md`.

## Hard rules

- Original kernel written in this repo. Never use Linux/BSD/Windows/other kernel
  code as the foundation. Bootloader (Limine), compilers, QEMU are tools, not the OS.
- C (C17, freestanding) for the kernel; assembly only where required. Rust only
  after an ADR approves it for a specific component.
- Never copy code from Wine, ReactOS, Linux, BSD or any project without a recorded
  license review in `LICENSES.md`. Compatibility is built from public documentation.
- Never claim a feature works unless it was built and tested. Use the status
  vocabulary: Implemented / Compiles / Unit-tested / Emulator-tested /
  Hardware-tested / Production-ready.
- Do not disable or delete failing tests. Do not add placeholder/stub code that
  pretends to work. Interfaces with `ENOSYS`-style "not implemented" returns are
  fine if clearly documented.
- Do not add a dependency or download without a one-line reason, a license note
  and (for binaries) a pinned version + SHA-256.
- Use the **current stable versions** of tools; check them, do not rely on memory.
- No real secrets or signing keys in git.
- Never write to a real disk or block device in tests or scripts — only
  disposable image files. Never claim "installation supported", "USB boot
  works" or "Secure Boot compatible" before the matching ROADMAP phase is
  VERIFIED.
- Code, comments, docs and commit messages are in English. Reports to the owner
  are in Uzbek (Latin).
