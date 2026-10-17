# SMY Compatibility

Status: **planning only.** No compatibility code exists. No Linux or Windows
program runs on SMY-OS. Full C0 research is Phase 9.

## 1. Approach

SMY-OS has its own native ABI. Linux and Windows programs are supported later
through **personalities**: each process has a personality flag that selects its
syscall table and loader behaviour.

- **Linux**: a Linux x86-64 syscall personality inside the kernel (idea similar
  to FreeBSD's Linuxulator; no code taken from anywhere).
- **Windows**: a PE32+ loader plus NT-like process services in the kernel and
  SMY-written user-mode libraries (`kernel32`/`ntdll` equivalents) built from
  public documentation. **No Wine or ReactOS code.**

## 2. Terms (must stay distinct)

| Term | Meaning | Example |
|---|---|---|
| API compatibility | Same function names/semantics at source level; needs recompiling | POSIX-like native library |
| ABI compatibility | Unmodified binaries run: same calling convention, syscall numbers, structure layouts | running a musl static ELF as-is |
| Translation | Calls of one API mapped to another at run time | Direct3D → SMY Graphics (future) |
| Emulation | Instructions of another CPU executed in software | not planned (x86-64 only) |

## 3. Milestones

| ID | Milestone | Acceptance criteria | Phase | Status |
|---|---|---|---|---|
| C0 | Architecture + executable-format research | This file completed + ADR on ELF/PE loading, personalities, API/ABI/translation/emulation | 9 | NOT STARTED |
| C1 | Executable-format inspection | Host tool `smy-inspect` parses ELF64 + PE32+ headers; unit tests with valid and malformed samples; no crash on 10 000 mutated inputs (ASan) | 9 | NOT STARTED |
| C2 | First native user-mode executable | Native SMY ELF runs in ring 3, makes a syscall, exits — QEMU test | 10 | NOT STARTED |
| C3 | First Linux executable | Static musl x86-64 ELF using `write`, `exit_group`, `brk`, `mmap`, `munmap`, `arch_prctl`, `set_tid_address` prints text and exits with the right code — QEMU test | 15 | NOT STARTED |
| C4 | First Windows executable | MinGW-w64 PE32+ console program calling `GetStdHandle`, `WriteFile`, `ExitProcess` prints text and exits — QEMU test | 24 | NOT STARTED |
| C5 | Basic runtime/library compatibility | Linux dynamic linking (musl `ld.so`) and PE DLL loading; documented test list passes | 38+ | NOT STARTED |
| C6 | Basic graphical applications | A defined simple GUI program per platform renders through SMY Graphics | 38+ | NOT STARTED |
| C7 | Broader compatibility testing | Published matrix of tested apps with pass/fail and reproducible scripts | 38+ | NOT STARTED |
| C8 | Graphics/multimedia/gaming research | Research report + prototypes; no performance claims without benchmarks | 38+ | NOT STARTED |

A milestone is complete only when its criteria pass in CI. One running program
does not mean "Linux compatible" or "Windows compatible".

## 4. Architectural consequences already decided

- Process model, VM API and file-descriptor/handle table are designed in
  Phases 10–11 so that both POSIX file descriptors and NT handles can be
  layered on them.
- Executable parsers live in `compat/executable_loader/` and are shared by host
  tools and the kernel (Phase 9).
- Syscall entry dispatches through a per-process table pointer (Phase 10),
  making personalities a table swap rather than a branch in every syscall.

## 5. Licence notes

Wine (LGPL-2.1+) and ReactOS (GPL-2.0+, parts LGPL) may be studied only via
their public design notes; no code is copied. Sources used for compatibility
work are listed per document in `docs/compatibility/` (from Phase 9).
