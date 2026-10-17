# ADR 0004 — Build system: GNU Make

- **Status:** Accepted
- **Date:** 2026-10-17
- **Phase:** 0

## Context

The build must produce: the kernel ELF, host unit-test binaries (with
sanitizers), a bootable hybrid ISO (`make iso`, Phase 2), disk images for
tests, and later user-space programs and an initramfs. It must be reproducible
(`SOURCE_DATE_EPOCH`, fixed file order) and easy to drive from CI and from a
cloud session with plain commands. Available: GNU Make 4.3, CMake 3.28.3,
Ninja 1.11.1.

## Decision

**GNU Make** (non-recursive, one top-level `Makefile` including per-directory
`*.mk` fragments).

- Out-of-tree outputs in `build/` (git-ignored); `make clean` removes only
  `build/`.
- Automatic header dependencies with `-MMD -MP`.
- Standard targets (introduced phase by phase): `all`, `kernel`, `test`
  (host unit tests), `iso`, `run` (QEMU), `check` (full sequence:
  build + tests + format + static analysis + QEMU tests), `format`,
  `format-check`, `lint`, `clean`.
- Toolchain variables (`CC`, `LD`, …) overridable from the command line.
- `V=1` shows full command lines.

## Alternatives

| Option | Advantages | Disadvantages | Verdict |
|---|---|---|---|
| **GNU Make** | Pre-installed everywhere; no generator step; shell-level control over ISO/image steps; common in OS projects | Easy to write badly (recursive make, missing deps) | **Chosen**, with the non-recursive rule |
| CMake + Ninja | Good IDE integration, fast | Cross-compiling freestanding kernels with custom linker scripts and image steps fights CMake's model; extra generator layer | Rejected |
| Meson | Clean syntax | Extra dependency; same cross-file complexity | Rejected |
| Shell scripts only | Simple | No incremental builds, no dependency tracking | Rejected |

## Consequences

- One command per action for the owner/CI (`make check`, `make iso`).
- If the tree grows to many user-space programs, revisit (a generated build
  could become simpler).
