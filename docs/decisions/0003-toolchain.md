# ADR 0003 — Toolchain: Clang + ld.lld targeting x86_64-unknown-none-elf

- **Status:** Accepted
- **Date:** 2026-10-17
- **Phase:** 0

## Context

A kernel needs a compiler that targets bare metal without a host libc. The
build runs in two places only: ephemeral cloud sessions and GitHub Actions
(both Ubuntu 24.04, see `docs/architecture/environment.md`). Building a GCC
cross-compiler takes 20–40 minutes and would have to be cached or repeated in
every CI run and every session.

Measured: Ubuntu 24.04 provides clang 18.1.3, ld.lld 18.1.3, clang-format and
clang-tidy 18.1.3 from its own archive.

## Decision

- Compiler: **clang** with `--target=x86_64-unknown-none-elf`.
- Linker: **ld.lld** with a custom linker script (higher-half kernel, Phase 2).
- Archiver/utilities: `llvm-ar`, `llvm-objcopy`, `llvm-nm`, `llvm-objdump`
  from the same LLVM version.
- Version: the **LLVM version shipped by Ubuntu 24.04 (currently 18.1.3)** in both
  environments. `tools/check-env.sh` (Phase 1) fails if the major version is
  older than 18 and prints the exact version in every CI log.
- Format and lint: `clang-format`, `clang-tidy` of the same LLVM version, plus
  `cppcheck` (Ubuntu package, 2.13.0).
- Host tests: the same clang targeting the host, with ASan/UBSan.

## Alternatives

| Option | Advantages | Disadvantages | Verdict |
|---|---|---|---|
| **Ubuntu clang + lld** | Already installed / one `apt` line; identical in session and CI; cross-target built in | Not the newest LLVM | **Chosen** |
| Newest LLVM from apt.llvm.org | Newest features and fixes | Extra apt repository and key in every session and CI run; version drift between runs unless pinned carefully | Deferred — revisit if a needed feature or a bug fix requires it |
| GCC cross-compiler (`x86_64-elf-gcc`) | Classic OSDev path | Must be built from source and cached; slow and fragile in ephemeral sessions | Rejected |
| Host GCC with `-ffreestanding` | Installed | Targets Linux ABI defaults; easy to pull in host assumptions | Rejected |

## Consequences

- No toolchain build step; CI setup is one `apt-get` line.
- "Current stable version" rule (`CLAUDE.md`): Ubuntu's LLVM is older than
  upstream's latest release. This is a deliberate trade for reproducibility
  across both environments. Re-check at Phase 1 and every 10 phases whether a
  newer pinned LLVM is worth adopting.
