# SMY-OS Status

Last updated: 2026-10-17 — Phase 2.

Status vocabulary (used exactly): **Implemented · Compiles · Unit-tested ·
Emulator-tested · Hardware-tested · Production-ready.** A passing build is not
proof of correctness or security.

## Summary

**The first SMY Kernel boots — in QEMU only.** `make iso` builds a
reproducible hybrid ISO (Limine 12.9.3 built from verified source + SMY
Kernel). It boots under BIOS and UEFI, from CD and as a raw disk (what a USB
stick looks like to firmware). The kernel writes its boot log to the serial
port, paints the boot framebuffer (dark background, blue square frame), prints
`SMY-OS: boot OK` and halts. Nothing else exists yet: no interrupts, no memory
management, no processes.

- Installation: **not supported.**
- USB boot: **not tested on a real USB stick or PC.** The ISO boots in QEMU when attached as a raw disk (USB simulation) — Emulator-tested only.
- Secure Boot: **not supported.**
- Hardware-tested: **nothing.**

## Phases

| Phase | Status | Notes |
|---|---|---|
| 0 — Foundation | VERIFIED | Docs, ADRs 0001–0006, environment report, link check; licence Apache-2.0 (ADR 0006) |
| 1 — Build system, toolchain, CI | VERIFIED | `make check` green locally and in CI (run 39066632132) |
| 2 — First boot | IN PROGRESS | see below; VERIFIED once CI `ci` is green on the pushed commit |
| 3 … 38 | NOT STARTED | see `ROADMAP.md` |

## Components

| Component | Status |
|---|---|
| Documentation (root docs, ADRs 0001–0008, ISO spec) | Implemented, link-checked |
| Build system (`make check`, `make iso`) | Implemented |
| Host unit-test harness | Implemented, 16 tests |
| Kernel string library | Unit-tested, Emulator-tested (linked into the booting kernel) |
| Number formatting (`core/fmt`) | Unit-tested, Emulator-tested |
| Kernel entry, Limine requests (base revision 6), linker layout with W^X segments | Emulator-tested (BIOS + UEFI) |
| COM1 16550 serial log (polled, loopback-probed) | Emulator-tested |
| Boot framebuffer fill (`drivers/bootfb`) | Unit-tested, Emulator-tested (pixels checked once via QEMU screendump) |
| `panic()` | Emulator-tested (negative test: injected panic is detected by the boot test) |
| Stack protector | Compiles (fixed canary; no test that triggers it yet — Phase 3) |
| Limine 12.9.3 (built from SHA-256-verified source) | Emulator-tested |
| Hybrid ISO: BIOS CD, BIOS disk, UEFI CD, UEFI disk | Emulator-tested (QEMU 8.2.2, SeaBIOS, OVMF 2024.02) |
| Reproducible ISO | Verified locally (two build directories, identical SHA-256) |
| Real hardware, real USB stick | not tested (Phase 36) |
| Design system | not started (Phase 6) |
| Compatibility (C0–C8) | not started (see `COMPATIBILITY.md`) |

## Last check results (Phase 2, cloud session 2026-10-17)

| Check | Command | Result |
|---|---|---|
| Environment | `tools/check-env.sh --all` | PASS, 0 warnings (no KVM → QEMU TCG) |
| Kernel link | `make kernel` | PASS; 4 PT_LOAD segments: requests RW, text RX, rodata R, data/bss RW — none W+X |
| Unit tests | `make test` | PASS: 16 run, 16 passed (ASan + UBSan) |
| Format / lint | `make format-check`, `make lint` | PASS |
| ISO | `make iso` | PASS: 3.4 MB hybrid ISO |
| Boot BIOS / CD | `make boot-test-bios-cd` | PASS in ≈1 s |
| Boot BIOS / raw disk | `make boot-test-bios-disk` | PASS in ≈1 s |
| Boot UEFI / CD | `make boot-test-uefi-cd` | PASS in ≈4 s |
| Boot UEFI / raw disk | `make boot-test-uefi-disk` | PASS in ≈4 s |
| Reproducibility | `tools/check-reproducible.sh` | PASS: identical SHA-256 from two build directories |
| Negative checks | see `TESTING.md` §3 | injected panic, hang and device path each rejected |
| CI `ci` workflow | GitHub Actions | pending (recorded after push) |

Boot log (UEFI, CD):

```
SMY-OS: SMY Kernel starting
SMY-OS: bootloader: Limine 12.9.3
SMY-OS: firmware: UEFI
SMY-OS: framebuffer: 1280x800 32bpp
SMY-OS: boot OK
```

## Known limitations

- Cloud session has no KVM; QEMU will run under TCG (slow) — see
  `docs/architecture/environment.md`.
- Ubuntu's LLVM 18 is used instead of the newest upstream LLVM (ADR 0003).
