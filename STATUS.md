# SMY-OS Status

Last updated: 2026-10-17 — Phase 1 (VERIFIED).

Status vocabulary (used exactly): **Implemented · Compiles · Unit-tested ·
Emulator-tested · Hardware-tested · Production-ready.** A passing build is not
proof of correctness or security.

## Summary

The build system, host unit-test harness, static analysis and CI exist. The
only kernel code is the memory/string library (`kernel/core/string.c`),
compiled with kernel flags into `build/kernel/libsmy-kernel.a`. **There is no
bootable kernel yet; nothing boots** (Phase 2).

- Installation: **not supported.**
- USB boot: **not supported / not tested.**
- Secure Boot: **not supported.**
- Hardware-tested: **nothing.**

## Phases

| Phase | Status | Notes |
|---|---|---|
| 0 — Foundation | VERIFIED | Docs, ADRs 0001–0006, environment report, link check; licence Apache-2.0 (ADR 0006) |
| 1 — Build system, toolchain, CI | VERIFIED | `make check` green locally and in CI (run 39066632132) |
| 2 … 38 | NOT STARTED | see `ROADMAP.md` |

## Components

| Component | Status |
|---|---|
| Documentation (root docs, ADRs 0001–0007) | Implemented, link-checked by `tools/check-links.py` |
| Build system (GNU Make, `make check`) | Implemented |
| Host unit-test harness (`tests/unit/harness`) | Implemented, used by 8 tests |
| Kernel string library: `memcpy`, `memmove`, `memset`, `memcmp`, `strlen` | Compiles (kernel flags), Unit-tested (host, ASan/UBSan) |
| Limine 12.9.3 source fetch (`tools/fetch-limine.sh`) | Implemented, SHA-256 verified; not built yet (Phase 2, ADR 0007) |
| SMY Kernel (bootable) | not started (Phase 2) |
| Design system | not started (Phase 6) |
| Compatibility (C0–C8) | not started (see `COMPATIBILITY.md`) |

## Last check results (Phase 1, cloud session 2026-10-17)

| Check | Command | Result |
|---|---|---|
| Environment | `tools/check-env.sh` | PASS, 0 warnings (clang/lld 18.1.3, cppcheck 2.13.0, QEMU 8.2.2; no KVM) |
| Limine fetch | `tools/fetch-limine.sh` | PASS: download + SHA-256 match; second run uses verified cache |
| Kernel objects | `make kernel` | PASS; `llvm-nm -u` on `string.o`: no undefined symbols; no `call` instructions (no self-calls from loop-to-memcpy conversion) |
| Unit tests | `make test` | PASS: 8 run, 8 passed, 0 failed (ASan + UBSan) |
| Format | `make format-check` | PASS (5 files) |
| Static analysis | `make lint` | PASS (clang-tidy, cppcheck) |
| Negative checks | see `TESTING.md` §3 | each injected fault made its check fail (exit ≠ 0) |
| Docs links | `python3 tools/check-links.py` | PASS |
| CI `ci` workflow | GitHub Actions | PASS: run 39066632132 on commit d8885d1 — check-env, fetch-limine, kernel, 8/8 unit tests, format, lint all green (~25 s) |

## Known limitations

- Cloud session has no KVM; QEMU will run under TCG (slow) — see
  `docs/architecture/environment.md`.
- Ubuntu's LLVM 18 is used instead of the newest upstream LLVM (ADR 0003).
