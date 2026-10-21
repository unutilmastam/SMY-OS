# SMY-OS Status

Last updated: 2026-10-21 — Phase 6 verified.

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
memory map); since Phase 5 it runs on its own page tables (W^X kernel
sections, NX, write-protect, SMEP/SMAP when available), on guard-paged
stacks, and has a kernel heap (`kmalloc`/`kfree`). It shows the brand boot
logo. Still missing: timers and interrupts, threads, processes, any user
interface.

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
| 4 — Physical memory manager | VERIFIED | CI run 39076695215 green (incl. 4 GiB test) |
| 5B — Brand identity | VERIFIED | concept A "Module" (ADR 0011); logo family approved by the owner 2026-10-21; CI run 39575540426 green (incl. boot-screen pixel test) |
| 5 — Virtual memory + heap | VERIFIED | CI run 39578027802 green (ktest 24/24 BIOS+UEFI, `-cpu max`, 5 panic reports, 4 GiB) |
| 6 — D1 Design system | VERIFIED | CI run 39580554634 green; monochrome themes, `#077479`, Inter + IBM Plex Mono approved by the owner 2026-10-21 |
| 7 … 38, 12B | NOT STARTED | see `ROADMAP.md` |

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
| Boot framebuffer fill + RLE image blit (`drivers/bootfb`) | Unit-tested, Emulator-tested (`make boot-screen`: logo pixels checked via QEMU screendump on BIOS and UEFI, in CI) |
| Kernel boot screen: brand boot logo on `#080B12` (rendered from SVG at build time, 96–256 px) | Emulator-tested (QEMU BIOS + UEFI); not yet seen by the owner in VirtualBox |
| GDT, TSS with IST stacks (#DF, NMI, #MC), IDT (vectors 0–31) | Unit-tested (encodings), Emulator-tested (ktest reads back GDTR/IDTR/TR) |
| Exception handling: `#UD #BP #PF #GP #DE` caught and resumed (ktest), 1000 in a row | Emulator-tested (BIOS + UEFI) |
| `#DF` on its IST stack (kernel stack overflow) | Emulator-tested (`make ktest-panic-stack-overflow`) |
| NMI, `#MC` on IST stacks | Implemented, **not tested** (no safe trigger) |
| Own page tables: direct map (2 MiB pages), kernel sections W^X, NX, CR0.WP, PAT write-combining framebuffer | Unit-tested (table logic), Emulator-tested (BIOS, UEFI, 4 GiB) |
| SMEP / SMAP | Emulator-tested with QEMU `-cpu max` (`make ktest-cpu-max`); off on CPUs without them |
| Guard-paged kernel stacks (main + IST) | Emulator-tested (guard read faults; overflow → #DF report) |
| Kernel heap (`kmalloc`/`kfree`, size-class slabs + page runs) | Unit-tested (incl. 30 000-step random test), Emulator-tested (20 000-step stress, no leak; double free → panic) |
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
| Logo concepts A Module, B Signal, C Prism (`design/brand/concepts/`) | Implemented (SVG, validated in CI); owner chose A (ADR 0011) |
| Logo family (22 SVGs, `design/brand/logos/`, manifest, guidelines) | Implemented, validated in CI (SVG rules, manifest, contrast, PNG exports 16–1024 px); approved by the owner 2026-10-21 |
| Brand colour tokens (`design/tokens/brand-colors.json`) | Implemented (base layer; UI themes follow in Phase 6) |
| Design tokens (`design/tokens/`: base ramps, 3 themes, foundations) | Implemented; 147 contrast pairs pass (CI `make design-check`) |
| Token code generator (`gen_tokens.py` → `smy_tokens.h`, `smy_tokens.css`) | Implemented; header compiled with -Werror in CI; files not committed (ADR 0013) |
| Fonts Inter 4.1, IBM Plex Mono 2.5.0 | Pinned by SHA-256, not committed; Uzbek Latin + Cyrillic coverage checked in CI |
| Theme review sheet (`build/brand/themes-preview.png`) | Generated in CI; approved by the owner (second, monochrome version) |
| Compatibility (C0–C8) | not started (see `COMPATIBILITY.md`) |

## Last check results (Phase 5, cloud session 2026-10-21)

| Check | Command | Result |
|---|---|---|
| Full local check | `make check` | PASS (exit 0) |
| Unit tests | `make test` | PASS: 61 run, 61 passed (9 paging + 7 heap new) |
| Format / lint | `make format-check`, `make lint` | PASS |
| Boot tests, boot screen | `make boot-test`, `make boot-screen` | PASS 4/4, logo pixels PASS on BIOS and UEFI |
| ktest suite | `make ktest-bios`, `make ktest-uefi` | PASS: 24 run, 24 passed on each firmware |
| SMEP/SMAP | `make ktest-cpu-max` | PASS: 24/24 with "SMEP on, SMAP on" |
| Big memory | `make ktest-big-memory` | PASS (direct map 4 099 MiB, 88 KiB of tables) |
| Panic reports | `make ktest-panic-*` (5) | PASS incl. stack overflow → #DF and double `kfree` |
| Negative checks | see `TESTING.md` §3 | 2 injected bugs, each detected |
| Reproducible ISO | `tools/check-reproducible.sh` | PASS |
| CI `ci` workflow | GitHub Actions | PASS: run 39578027802 on commit af9157b |

## Earlier check results (Phase 5B step 2, cloud session 2026-10-21)

| Check | Command | Result |
|---|---|---|
| Full local check | `make check` | PASS (exit 0) |
| Unit tests | `make test` | PASS: 45 run, 45 passed (3 new RLE blit tests) |
| Format / lint | `make format-check`, `make lint` | PASS |
| Design assets | `make brand-check`, `make brand` | PASS: 34 SVGs valid, 22 logos in manifest, 8 contrast pairs, 132 PNG exports |
| Boot screen | `make boot-screen` | PASS on BIOS and UEFI (1280×800: logo pixels present and placed, rest VOID) |
| Boot tests, ktest, panic reports, 4 GiB | `make boot-test`, `make ktest` | PASS |
| Reproducible ISO | `tools/check-reproducible.sh` | PASS (after mapping the build-directory path of the generated logo source) |
| Link check | `tools/check-links.py` | PASS: 0 broken links |
| CI `ci` workflow | GitHub Actions | PASS: run 39575540426 on commit b95c93a (after two fixes for the label font on the runner) |

## Earlier check results (Phase 4, cloud session 2026-10-18)

| Check | Command | Result |
|---|---|---|
| Unit tests | `make test` | PASS: 42 run, 42 passed (ASan + UBSan) |
| Format / lint | `make format-check`, `make lint` | PASS |
| Boot tests | `make boot-test` | PASS 4/4 |
| ktest suite | `make ktest-bios`, `make ktest-uefi` | PASS: 13 run, 13 passed on each firmware |
| Big memory | `make ktest-big-memory` | PASS: 4 047 MiB usable, 1 036 060 pages allocated and freed (≈29 s under TCG) |
| Panic reports | `make ktest-panic-*` | PASS |
| Negative checks | see `TESTING.md` §3 | 3 injected allocator bugs, each detected |
| CI `ci` workflow | GitHub Actions | PASS: run 39076695215 on commit 7eac9e8 |

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
