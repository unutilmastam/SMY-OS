# ADR 0009 — In-kernel self-tests, selected through an SMBIOS-supplied boot config

- **Status:** Accepted
- **Date:** 2026-10-18
- **Phase:** 3

## Context

Phase 3 needs tests that run inside the kernel (exceptions, descriptor
tables) and report a result automatically in CI. The kernel must learn that
it runs a test (a command line option) and report pass/fail to the test
runner. The normal boot path must not change.

## Decision

1. **ktest framework in the kernel** (`core/ktest.h`): tests registered in a
   `.ktest` linker section; selected with `smy.ktest=all|<name>`; results on
   serial; exit through QEMU's `isa-debug-exit` (port `0xF4`, status 33 =
   pass, 35 = fail). The kernel writes that port **only** in ktest mode.
2. **Tests ship in every kernel build** (about 3 KiB of code). There is one
   kernel image, so what CI tests is exactly what boots; a separate test
   build could differ. Running tests requires control of the boot command
   line, which already means full control of the machine.
3. **The option is delivered through SMBIOS**, not a second ISO: the runner
   passes a Limine configuration as an SMBIOS type 11 OEM string
   (`limine:config:`), which Limine prefers over the config file on disk.
   The config uses `fslabel(SMY-OS):` because `boot():` is undefined for
   SMBIOS configs on BIOS. Verified on QEMU 8.2 with SeaBIOS and OVMF.

## Alternatives

| Option | Why not chosen |
|---|---|
| Separate test ISO per selection | More images to build and keep reproducible; tests would not run the release image |
| Interactive Limine menu entry | Needs keyboard input from the test harness |
| A test-only kernel build (`#ifdef`) | Tested binary differs from shipped binary |
| Kernel module loaded from the ISO with options | No module loading until Phase 10 |

## Consequences

- One ISO for boot tests, ktests and release; CI and the owner use the same
  file.
- Limine honours SMBIOS OEM strings on any machine. Someone who can change a
  machine's SMBIOS tables (firmware/hypervisor level) can change the boot
  configuration — that party already controls the boot. Relevant for Secure
  Boot (Phase 37): Limine's config hash enrolment must cover this path;
  recorded in `THREAT_MODEL.md`.
- On real hardware the ktest exit falls back to halting (no `isa-debug-exit`
  device).
