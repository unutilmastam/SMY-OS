# Contributing to SMY-OS

SMY-OS is developed phase by phase (`ROADMAP.md`). Most work is done by
Claude Code sessions driven by the owner; human contributors follow the same
rules. `CLAUDE.md` and `docs/MASTER_PROMPT.md` are binding.

## 1. Before you change anything

1. Read `CLAUDE.md`, `docs/MASTER_PROMPT.md`, `ROADMAP.md`, `STATUS.md`, and the
   ADRs (`docs/decisions/`) for the area you touch.
2. Work only on the current phase. One phase per change set.
3. Significant decisions need an ADR (`docs/decisions/0000-template.md`).

## 2. Originality and licences

- Write the code yourself. Never copy from Linux, BSD, Windows, Wine, ReactOS
  or any other project without a recorded licence review in `LICENSES.md`.
- Compatibility work uses public documentation only.
- New dependency → one-line reason, licence, and (binaries) pinned version +
  SHA-256, recorded in `LICENSES.md`.
- The project licence is Apache-2.0 (ADR 0006). Contributions are accepted
  under the same licence. Every source file starts with
  `SPDX-License-Identifier: Apache-2.0` and
  `Copyright 2026 The SMY-OS Authors`.

## 3. Code style

- C17, freestanding in the kernel (ADR 0002). Assembly: clang integrated
  assembler, `.intel_syntax noprefix`, `.S` files.
- Formatting: `clang-format` with the repository `.clang-format` (Phase 1).
  `make format-check` must pass.
- Naming: `snake_case` for functions and variables, `UPPER_SNAKE` for macros
  and constants, subsystem prefix on public symbols (`pmm_alloc_page`,
  `vfs_open`). Types: `snake_case_t` only for opaque handles; structs are
  `struct name`.
- Headers: include guards `SMY_<PATH>_H`; public headers contain only the
  interface.
- No VLAs, no recursion in interrupt context, fixed-width types for hardware
  structures, `static` for everything not exported.
- Comments explain *why*, not *what*. English only.

## 4. Compiler warnings (all are errors)

```
-std=c17 -Wall -Wextra -Werror -Wpedantic -Wshadow -Wconversion
-Wsign-conversion -Wstrict-prototypes -Wmissing-prototypes -Wvla
-Wcast-align -Wnull-dereference -Wdouble-promotion -Wformat=2
```

Kernel-only additions: `--target=x86_64-unknown-none-elf -O2 -g
-ffreestanding -nostdlib -fno-builtin -fno-pic -fno-pie -mcmodel=kernel
-mno-red-zone -mno-sse -mno-sse2 -mno-mmx -mno-80387 -fno-omit-frame-pointer
-fstack-protector-strong -fno-asynchronous-unwind-tables -fno-lto`.

Host unit tests: `-O1 -g -fno-omit-frame-pointer
-fsanitize=address,undefined -fno-sanitize-recover=all`.

The authoritative list is `mk/toolchain.mk`; this section is updated with it.

Static analysis: `.clang-tidy` (bugprone, cert, clang-analyzer, misc,
performance, portability; disabled checks are listed with reasons in the
file) and `cppcheck --enable=warning,style,performance,portability`. A
deliberate exception is written as `// cppcheck-suppress <id>` or
`// NOLINT(<check>)` on the line, with a comment saying why.

## 5. Tests are mandatory

- New portable logic → host unit test (ASan + UBSan).
- New kernel behaviour → QEMU test with a timeout (a hang is a failure).
- Never disable, delete or weaken a failing test to get green.
- Never claim a result you did not observe. Use the status vocabulary in
  `STATUS.md`.

Full sequence before committing (from Phase 1): `make check`.

## 6. Commits

- Small, focused commits; English imperative subject line ≤ 72 characters,
  e.g. `kernel: add 16550 serial driver`.
- Body explains why, lists tests run and their results.
- Phases end with a commit + push to `main`. No force-push, no history
  rewriting.
- No secrets, keys or real-disk paths in the repository.

## 7. Review checklist

- [ ] Phase scope respected; no unrelated changes.
- [ ] Warnings clean, format clean, static analysis clean.
- [ ] Tests added and passing locally (or stated which only run in CI).
- [ ] Docs updated: `STATUS.md`, `TESTING.md`, component docs, ADR if needed.
- [ ] Licences recorded for any new third-party material.
- [ ] User-space input validated at every kernel boundary.
