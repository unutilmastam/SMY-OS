# SMY-OS Status

Last updated: 2026-10-18 — Phase 4.

Status vocabulary (used exactly): **Implemented · Compiles · Unit-tested ·
Emulator-tested · Hardware-tested · Production-ready.** A passing build is not
proof of correctness or security.

## Summary

The SMY Kernel boots in QEMU (BIOS and UEFI, CD and raw disk) and in the
owner's VirtualBox. Since Phase 3 it installs its own GDT, TSS (with IST
stacks) and IDT, catches every CPU exception (vectors 0–31) and reports
unhandled ones as a panic with registers and a stack trace, logs with levels,
reads its command line, and runs in-kernel self-tests (`make ktest`). Since
Phase 4 it manages physical memory (page-frame allocator over the Limine
memory map). Still missing: its own page tables and heap, timers and
interrupts, processes, any user interface.

- Installation: **not supported.**
- USB boot: **not tested on a real USB stick or PC.** The ISO boots in QEMU when attached as a raw disk (USB simulation) — Emulator-tested only.
- Secure Boot: **not supported.**
- Hardware-tested: **nothing.**

## Phases

| Phase | Status | Notes |
|---|---|---|
| 0 — Foundation | VERIFIED | Docs, ADRs 0001–0006, environment report, link check; licence Apache-2.0 (ADR 0006) |
| 1 — Build system, toolchain, CI | VERIFIED | `make check` green locally and in CI (run 39066632132) |
| 2 — First boot | VERIFIED | 4/4 boot paths and reproducibility green in CI (runs 39068650073, 39069271282) |
| 3 — CPU foundation | VERIFIED | ktest 10/10 on BIOS and UEFI, 3 panic reports, boot 4/4, reproducible — CI run 39075758514 |
| 4 — Physical memory manager | IN PROGRESS | see below; VERIFIED once CI `ci` is green on the pushed commit |
| 5 … 38, 5B, 12B | NOT STARTED | see `ROADMAP.md` |

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
| GDT, TSS with IST stacks (#DF, NMI, #MC), IDT (vectors 0–31) | Unit-tested (encodings), Emulator-tested (ktest reads back GDTR/IDTR/TR) |
| Exception handling: `#UD #BP #PF #GP #DE` caught and resumed (ktest), 1000 in a row | Emulator-tested (BIOS + UEFI) |
| `#DF`, NMI, `#MC` on IST stacks | Implemented, **not tested** (no safe trigger before Phase 5) |
| `panic()` and exception panic report (registers, CR2 decode, stack trace) | Emulator-tested (3 panic ktests match the report lines) |
| Kernel log with levels, printf-subset formatter | Unit-tested (formatter), Emulator-tested |
| Kernel command line (`smy.ktest`, `smy.loglevel`) | Unit-tested, Emulator-tested |
| Frame-pointer stack walker | Unit-tested (fake stacks), Emulator-tested (real frames) |
| ktest framework | Emulator-tested (16 tests; negative checks in `TESTING.md`) |
| Physical memory manager (two-bitmap page-frame allocator, ADR 0010) | Unit-tested (incl. model-based random test), Emulator-tested: every free page allocated and returned without leak at 256 MiB (BIOS, UEFI) and 4 GiB (UEFI) |
| Stack protector | Compiles (fixed canary; no test that triggers it yet) |
| Limine 12.9.3 (built from SHA-256-verified source) | Emulator-tested |
| Hybrid ISO: BIOS CD, BIOS disk, UEFI CD, UEFI disk | Emulator-tested (QEMU 8.2.2, SeaBIOS, OVMF 2024.02) |
| Reproducible ISO | Verified: two builds at different checkout paths give identical SHA-256 (locally and in CI) |
| Owner check in VirtualBox (Windows 11 laptop, BIOS mode, VM type "Other/Unknown 64-bit", 512 MB, ISO as optical drive) | Emulator-tested by the owner on 2026-10-18: boot screen (dark background, blue frame) shown; reported with a photo. VirtualBox version not recorded; serial log not captured |
| Real hardware, real USB stick | not tested (Phase 36) |
| Design system | not started (Phase 6) |
| Compatibility (C0–C8) | not started (see `COMPATIBILITY.md`) |

## Last check results (Phase 4, cloud session 2026-10-18)

| Check | Command | Result |
|---|---|---|
| Unit tests | `make test` | PASS: 42 run, 42 passed (ASan + UBSan) |
| Format / lint | `make format-check`, `make lint` | PASS |
| Boot tests | `make boot-test` | PASS 4/4 |
| ktest suite | `make ktest-bios`, `make ktest-uefi` | PASS: 13 run, 13 passed on each firmware |
| Big memory | `make ktest-big-memory` | PASS: 4 047 MiB usable, 1 036 060 pages allocated and freed (≈29 s under TCG) |
| Panic reports | `make ktest-panic-*` | PASS |
| Negative checks | see `TESTING.md` §3 | 3 injected allocator bugs, each detected |
| CI `ci` workflow | GitHub Actions | pending (recorded after push) |

Memory map details: [docs/kernel/memory.md](docs/kernel/memory.md).
Example panic report: [docs/kernel/exceptions.md](docs/kernel/exceptions.md).

## Known limitations

- Bootloader- and ACPI-reclaimable memory is not yet returned to the
  allocator (≈45 MiB under OVMF at 256 MiB). The allocator has no lock.

- No guard pages: a kernel stack overflow corrupts memory silently until
  Phase 5. `#DF`/NMI/`#MC` handlers are installed but untested.
- Stack traces print addresses only (resolve with `llvm-addr2line`); an
  assembly leaf function hides its C caller.

- Cloud session has no KVM; QEMU will run under TCG (slow) — see
  `docs/architecture/environment.md`.
- Ubuntu's LLVM 18 is used instead of the newest upstream LLVM (ADR 0003).
