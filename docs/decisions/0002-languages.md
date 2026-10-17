# ADR 0002 — Languages: C17 freestanding, GNU assembler (Intel syntax), Rust deferred

- **Status:** Accepted
- **Date:** 2026-10-17
- **Phase:** 0

## Context

The master prompt (§3.3) fixes C17 freestanding for the kernel, assembly only
where required with **one** chosen syntax, and Rust only after an ADR for a
specific component. This ADR records the details: the C dialect rules, the
assembler and syntax, and the conditions for Rust.

## Decision

### C

- **C17** (`-std=c17`), freestanding (`-ffreestanding -nostdlib`), no compiler
  extensions except those wrapped in macros in one header
  (`kernel/core/compiler.h`: `__attribute__((packed))`, `noreturn`, section
  placement, `__builtin_expect`).
- Kernel-mode flags (finalised in Phase 1/2): `-mno-red-zone`,
  `-mcmodel=kernel`, `-mno-sse -mno-sse2 -mno-mmx -mno-80387`
  (no FPU/SIMD state in kernel), `-fno-builtin` where it matters,
  `-fno-omit-frame-pointer` (stack traces), `-fstack-protector-strong`.
- Warnings: `-Wall -Wextra -Werror -Wpedantic -Wshadow -Wconversion
  -Wstrict-prototypes -Wmissing-prototypes -Wvla -Wcast-align
  -Wnull-dereference` (exact list kept in `CONTRIBUTING.md`).
- No VLAs, no recursion in interrupt paths, fixed-width integer types for
  hardware structures.

### Assembly

- **Clang's integrated assembler (GNU `as` dialect) with
  `.intel_syntax noprefix`**, in `.S` files (C preprocessor available).
- Used only for: entry stubs, interrupt/exception stubs, context switch,
  `syscall` entry, and instructions without a C intrinsic. Small single
  instructions may use inline `asm` in C (AT&T operands are not used in
  `.S` files; inline asm follows the compiler's default).
- NASM is **not** used: one fewer tool in the build, and the integrated
  assembler shares the C preprocessor and object format with no extra step.

### Rust

- **Deferred.** Not used in the kernel in phases 0–16.
- A later ADR may approve Rust for a specific component, typically a parser of
  untrusted input (e.g. filesystem or network parsing, PE/ELF loader hardening).
  Conditions: stable toolchain pinned by version and checksum, `no_std`, a clean
  C ABI boundary, CI support, and a measurable benefit.

### Host tools

- Host tests: C17 with the host clang and `-fsanitize=address,undefined`.
- Scripts: POSIX `sh` (`#!/bin/sh`, `set -eu`) for simple wrappers; Python 3
  (standard library only) where logic is non-trivial. The language is noted at
  the top of each tool.

## Alternatives

| Option | Why not chosen |
|---|---|
| NASM (Intel syntax) | Extra tool, separate preprocessor, no gain over the integrated assembler |
| AT&T syntax | Harder to read and compare against Intel/AMD manuals, which use Intel syntax |
| C11 / C23 | C11 lacks nothing we need but C17 fixes defects; C23 support in clang 18 is incomplete |
| C++ | Larger runtime surface (exceptions, RTTI, global constructors) for little benefit in a kernel of this size |
| Rust from day one | Toolchain and `unsafe` boundary cost before anything boots; the prompt requires an ADR per component |

## Consequences

- One compiler driver (`clang`) for C and assembly — simpler build and CI.
- Memory-safety relies on discipline, sanitized host tests, fuzzing and static
  analysis until (and unless) Rust is approved for specific parts.
- Revisit Rust when Phase 9 (untrusted-input parsers) or Phase 22 (network
  stack) starts.
