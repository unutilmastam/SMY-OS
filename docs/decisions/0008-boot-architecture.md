# ADR 0008 — Boot architecture: hybrid ISO, UEFI primary, BIOS secondary

- **Status:** Accepted
- **Date:** 2026-10-17
- **Phase:** 2

## Context

The installer specification (`docs/INSTALLER_PROMPT.md` §1–§3) requires one
reproducible command that produces an image which boots from optical media
and when written raw to a USB stick, on UEFI (primary) and legacy BIOS
(secondary). Phase 2 is the first milestone: the smallest bootable ISO with
the current kernel, booted in an emulator with recorded results. Limine is the
bootloader (ADR 0005), built from source (ADR 0007).

## Decision

- **One hybrid ISO** (`make iso`) for all four paths: BIOS CD, BIOS disk,
  UEFI CD, UEFI disk. Layout and options: `docs/install/iso-spec.md`.
- **UEFI x86-64 is the primary path**; BIOS is kept because Limine provides it
  at no extra cost and older PCs need it. Both are tested on every push.
- **Kernel entry**: the Limine protocol, base revision 6 (latest in 12.9.3).
  Requests sit between start/end markers in a dedicated `.limine_requests`
  segment (read-write); text, rodata and data are separate segments with W^X
  permissions. Higher-half link address `0xffffffff80000000`.
- **First output channel**: COM1 16550 UART (polled), because the serial log is
  what automated tests can read. The boot framebuffer is painted as a visible
  sign of life, but tests do not depend on it in Phase 2.
- **Boot test contract**: the kernel prints `SMY-OS: boot OK`; the test passes
  on that line (and the expected firmware type), fails on `SMY-OS: PANIC`, an
  early QEMU exit, or a timeout. The kernel does **not** write QEMU's
  `isa-debug-exit` port in normal boots — that port is a QEMU test device and
  should not be poked on real hardware. `isa-debug-exit` is introduced with
  the in-kernel self-test mode (Phase 3), enabled only by a test boot option.
- **Architecture boundary**: generic code calls `arch/cpu.h`
  (`arch_halt_forever`); x86 specifics stay in `kernel/arch/x86_64/`.
  Drivers for x86-only legacy hardware (port I/O, e.g. the 16550 UART) may
  include `arch/x86_64/io.h`.

## Alternatives

| Option | Why not chosen |
|---|---|
| Separate ISO and USB images | Two artefacts to build, test and document; Limine's hybrid ISO covers both |
| UEFI only | Drops older PCs; BIOS costs nothing extra with Limine and is tested automatically |
| Framebuffer text as the test channel | Needs a font and screenshot comparison; serial is simpler and exact (screenshot tests come in Phase 18) |
| Always exit via `isa-debug-exit` after boot | Writes to an arbitrary I/O port on real hardware |

## Consequences

- Every later phase must keep `make iso` and all four boot tests green.
- A real USB stick and a real PC are still untested (Phase 36); "USB boot
  works" may not be claimed before then (`CLAUDE.md`).
- Secure Boot is not supported (Limine unsigned; Phase 37).
