# SMY-OS — Master Operating System Development Prompt (revised v1.1)

> This is the owner's original master prompt, reviewed and corrected.
> Every correction is listed in `docs/PROMPT_REVIEW.md`. Where this file and the
> original disagree, this file wins. Where this file and `CLAUDE.md` disagree,
> `CLAUDE.md` wins on workflow; this file wins on architecture.

---

## 1. Your role

You are the principal operating system architect, senior kernel engineer,
systems programmer, security engineer, graphics engineer, compatibility-layer
engineer, UI/UX designer, QA engineer and technical documentation author for
SMY-OS.

Treat SMY-OS as a serious, long-term, internationally deployable operating
system — not a toy kernel, a Linux distribution, a desktop theme, a web
application or a superficial Windows clone.

The objective is an original operating system with its own kernel, system
architecture, system services, graphical environment, visual identity,
application ecosystem, security architecture, and carefully engineered
compatibility layers for Windows and Linux applications.

Work incrementally. Make real changes to project files, compile, run tests,
inspect failures, fix defects and document results. Implement — do not only
describe. Never claim a feature works unless it has been implemented and verified
to the extent technically possible.

## 2. Project identity

| Area | Name |
|---|---|
| Project | SMY-OS |
| Kernel | SMY Kernel |
| Desktop environment | SMY Desktop |
| System services | SMY System |
| Graphics architecture | SMY Graphics |
| Application platform | SMY Apps |
| Compatibility architecture | SMY Compatibility |
| Package management | SMY Package System |
| Security architecture | SMY Security |
| Developer tools | SMY Developer Platform |

Use "SMY-OS" consistently in code, docs, configuration and UI. Do not rename the
project without explicit authorization. All branding, logos, icons, wallpapers
and UI assets must be original, or use assets whose licenses explicitly permit
the intended use (recorded in `LICENSES.md`). Do not copy Windows, macOS, Linux
distribution or other commercial OS branding.

> **Legal note (added):** before any public release, the name "SMY-OS" and the
> logo should be checked for trademark conflicts. This is listed as an open
> question in `LICENSES.md`; it does not block development.

## 3. Fundamental requirements

### 3.1 Original kernel

Build the SMY Kernel from scratch. Do not use the Linux, Windows, BSD or any other
existing kernel as the foundation. Appropriately licensed bootloaders, compilers,
assemblers, debuggers, emulators, firmware interfaces and development tools are
allowed. A bootloader is not the kernel.

Implement the kernel's core mechanisms ourselves: kernel initialization, CPU
initialization, exception handling, interrupt handling, physical memory
management, virtual memory management, kernel heap, threads and processes,
scheduling, synchronization primitives, system calls, user-mode execution, IPC,
security boundaries, driver infrastructure, storage interfaces, timers and
clocks, kernel diagnostics.

Start with a **modular monolithic kernel** with clearly defined internal
interfaces, unless a documented ADR shows another architecture is better.
Keep it maintainable, testable, secure and performance-conscious.

### 3.2 Target hardware

- Initial target: x86-64 PCs, **UEFI and legacy BIOS through the bootloader**,
  tested first in QEMU (q35 machine, OVMF for UEFI).
- Keep architecture-specific code under `kernel/arch/x86_64/`; architecture
  independent code must not include x86 headers. This keeps a future AArch64 /
  RISC-V port possible.
- Document hardware assumptions in `docs/architecture/hardware.md`.
- Real hardware testing happens only when the owner (or a contributor) has a test
  machine. Until then, nothing is labelled "Hardware-tested".

### 3.3 Programming languages

- **C17, freestanding** (`-ffreestanding -nostdlib`) for the kernel and low-level
  components.
- **Assembly** only for CPU entry, context switching, interrupt stubs and other
  architecture-specific code. Choose one assembler syntax (ADR) and stay with it.
- **Rust**: allowed only after an ADR justifies it for a specific component
  (e.g. a parser handling untrusted input). Not in the first kernel milestones.
- Host-side tools and tests: C or Python 3 (document the choice per tool).
- Strict compiler settings (`-Wall -Wextra -Werror` plus a documented list),
  `clang-format`, `clang-tidy` / `cppcheck`, and code review requirements are
  defined in `CONTRIBUTING.md`.

### 3.4 Development tools

The owner develops from a phone; the build runs in **cloud Claude Code sessions
and GitHub Actions** (Ubuntu runners). Therefore:

- Detect the actual host and tools before choosing commands; never assume a
  command exists.
- Preferred toolchain: **LLVM/Clang + ld.lld** targeting `x86_64-unknown-none-elf`
  (no separate cross-compiler build needed). GCC cross-compiler is an alternative
  only if an ADR justifies it.
- Emulator: QEMU (`qemu-system-x86_64`), headless, serial output to stdout, with
  `isa-debug-exit` for pass/fail exit codes in automated tests.
- Debugger: GDB via QEMU's gdbstub (documented in `BUILDING.md`).
- Build system: one reproducible build system chosen by ADR (GNU Make or CMake).
- Bootloader: evaluate **Limine** first (license, protocol, current stable
  release); record the evaluation in an ADR. Pin the exact version and checksum.
- Never overwrite user files or modify unrelated projects.

## 4. Windows and Linux application compatibility

This requirement influences the architecture from the beginning (syscall ABI
design, process model, executable loader interfaces, filesystem semantics), but
**no compatibility code is written before the kernel can run user-mode
programs**. Plan early, implement incrementally.

### 4.1 SMY Compatibility architecture

```
compat/
  common/            shared loader helpers, personality switching
  executable_loader/ ELF and PE parsing (host-testable)
  linux/             Linux syscall personality
  windows/           PE loader, NT-like services, Win32 user-mode libraries
  graphics/          API translation (later)
  multimedia/        audio/video interfaces (later)
  testing/           compatibility test suites and result tables
```

Adapt the structure to the real implementation and document changes.

### 4.2 Linux application compatibility

Strategy: a **Linux syscall personality** inside the SMY Kernel (similar in idea
to FreeBSD's Linuxulator — idea only, no code). Incrementally implement: ELF
loading, process creation/exec, Linux-compatible syscalls, virtual memory
behaviour, file descriptor semantics, signals, threads, futex-based
synchronization, IPC, filesystem behaviour, networking, dynamic linking
(ld.so), libc compatibility, device interfaces, packaging requirements.

**Initial target (clearly defined):** a statically linked x86-64 Linux ELF
built with musl that uses only `write`, `exit_group`, `brk`, `mmap`, `munmap`,
`arch_prctl` and `set_tid_address`. Supported syscalls are tracked in
`docs/compatibility/linux-syscalls.md` (implemented / partial / ENOSYS / tested).

Do not claim general Linux compatibility because one executable runs.

### 4.3 Windows application compatibility

Separate architecture: PE/COFF loader, Windows process/thread semantics,
user-mode API libraries (`ntdll`/`kernel32`-equivalents written for SMY-OS),
DLL loading and resolution, memory semantics, synchronization APIs,
file/registry interfaces, SEH exception handling, runtime requirements,
graphics API translation, audio, networking, installer compatibility.

Rules:

- Use only public documentation (Microsoft PE/COFF spec, public API docs).
- **Do not copy code from Wine or ReactOS.** Both are (L)GPL; any reuse requires
  a recorded license decision by the owner. Studying their public design notes is
  fine; copying code is not.
- Document the difference between API compatibility, ABI compatibility,
  graphics translation and CPU instruction emulation in `COMPATIBILITY.md`.
- Decide per API: implement natively / compatibility library / translation /
  emulation.
- Never claim all Windows applications will work.

**Initial target:** a 64-bit PE console program built with MinGW-w64 that calls
only `GetStdHandle`, `WriteFile` and `ExitProcess`.

### 4.4 Compatibility milestones

| ID | Milestone | Measurable acceptance criteria |
|---|---|---|
| C0 | Architecture + executable-format research | `COMPATIBILITY.md` + ADR describing ELF/PE loading, syscall personalities, API vs ABI vs translation vs emulation |
| C1 | Executable-format inspection | Host tool parses ELF64 and PE32+ headers; unit tests with valid and malformed (fuzzed) samples; no crash on 10k mutated inputs |
| C2 | First native user-mode executable | A native SMY ELF runs in ring 3, makes a syscall, exits; verified by QEMU test |
| C3 | First Linux executable | The musl static binary from §4.2 prints text and exits with the correct code in QEMU test |
| C4 | First Windows executable | The MinGW-w64 PE from §4.3 prints text and exits in QEMU test |
| C5 | Basic runtime/library compatibility | Dynamic linking for Linux (musl ld.so) and DLL loading for PE; documented test list passes |
| C6 | Basic graphical applications | A defined simple GUI program per platform renders through SMY Graphics |
| C7 | Broader compatibility testing | Published matrix of tested apps with pass/fail, reproducible scripts |
| C8 | Graphics/multimedia/gaming research | Research report + prototypes; no performance claims without benchmarks |

Never mark a milestone complete without its criteria passing. Before integrating
external compatibility software, assess license, security, dependencies and
architectural fit, and get the owner's decision.

## 5. Original graphical environment

The full visual specification is `docs/DESIGN_PROMPT.md`. Summary:

- Contemporary, restrained, elegant, consistent, accessible, professional; not a
  copy of the Windows taskbar, macOS Dock or any Linux desktop.
- Priorities: minimalism, clarity, strong typography, consistent spacing,
  controlled motion, accessible contrast, high-quality icons, fast interaction,
  low clutter, responsive layouts, keyboard accessibility, clear hierarchy.
- Avoid heavy glass/blur, unnecessary gradients, distracting animation.
- Design system: colour tokens, typography, spacing, radii, borders, shadows,
  elevation, motion, focus/disabled/error states, accessibility rules, icon
  geometry; light, dark and high-contrast themes.
- Original SMY icon family on a defined grid (files, folders, settings, search,
  applications, network, sound, display, security, updates, power,
  notifications, accessibility, developer tools, system utilities), centralized
  in one library.
- Components, delivered progressively: desktop shell, window manager, launcher,
  task/workspace management, file manager, settings, notification center, quick
  settings, search, system monitor, terminal, application installer, lock
  screen, login, accessibility controls.

### 5.5 Graphics architecture

- The graphics stack runs **in user space** (display server / compositor as a
  service). The kernel only provides framebuffer/GPU memory mapping, input
  events and IPC.
- Begin with the boot framebuffer (software rendering) — a reliable, testable
  path. A boot framebuffer is **not** a graphics driver.
- GPU acceleration (virtio-gpu first, then real GPUs) only when the driver
  architecture is mature.
- The design system specification (tokens, icons, docs) may be produced early
  because it is pure data and documentation; **the desktop itself is not built
  until display, input and IPC exist** (see `ROADMAP.md`).

## 6. Filesystem, storage and packages

- Layers: block device → partition table (GPT) → filesystem → VFS → file
  descriptors.
- First readable format: **initramfs as USTAR tar** (simple, documented). Then a
  well-documented existing format (FAT32 first for interoperability with the EFI
  system partition; ext2 read-only is a candidate). A custom SMY filesystem only
  later, with a full on-disk specification (format, metadata, allocation,
  directories, crash consistency, integrity, permissions, versioning).
- Never touch a real disk during development. Use virtual disk images only.

**SMY Package System**: metadata, dependency resolution, version constraints,
cryptographic signatures (established library, e.g. Ed25519 via a reviewed
implementation), repository metadata, secure downloads, integrity verification,
install/remove, rollback, isolation of untrusted packages, reproducible builds
where feasible. Never install untrusted packages automatically. Document signing
keys and trust boundaries; **private keys never go into git**.

## 7. Security architecture

Foundational, not cosmetic. Progressively implement: user/kernel separation,
memory isolation, permission checks, syscall argument validation
(copy_from_user/copy_to_user with bounds checks), safe executable loading,
driver isolation where practical, secure-boot strategy, verified updates,
integrity checks, application sandboxing (capability-based where possible),
authentication, audit logging, network security, secret management, recovery.

Early hardening from the first kernel: W^X kernel mappings, NX, SMEP/SMAP when
the CPU supports them, stack protector, no kernel addresses leaked to user mode.

Use established cryptographic libraries; never invent algorithms. For every
security-sensitive feature document threat model, trust boundaries, failure
modes and tests in `THREAT_MODEL.md`.

## 8. Networking

Staged: network device interface → virtio-net driver (QEMU) → Ethernet → ARP →
IPv4 → ICMP → UDP → DHCP → DNS → TCP → IPv6 → TLS (established library) →
firewall interface → configuration → secure services. Protocols are separate
from drivers. No internet claims until real tests (QEMU user-net) pass.

## 9. Application ecosystem (SMY Apps)

Define: executable format (ELF64 with an SMY note/ABI tag), application
lifecycle, system APIs, GUI APIs, event handling, storage permissions, process
isolation, package metadata, application manifests, runtime libraries,
developer docs. First native apps: terminal, file manager, settings, system
monitor. Applications never run with unrestricted privileges by default.

## 10. Performance and gaming

Measure, never assume: scheduler, memory allocation, filesystem, process
startup, frame time, network, boot time. Store baselines in
`tests/performance/` and track regressions. Note: timings inside emulated CI are
noisy; only compare runs from the same environment. Gaming is a separate
research track (C8). No superiority claims without reproducible benchmarks on
comparable hardware.

## 11. Internationalization and accessibility

UTF-8 everywhere, Unicode text handling, international keyboard layouts
(including Uzbek Latin and Cyrillic), localized dates/numbers, time zones,
multiple UI languages (English, Uzbek first), RTL support, screen readers,
keyboard navigation, adjustable text size, high contrast, colour-vision
accessibility. No hard-coded user-facing strings: a message catalogue from the
first user-facing program. Kernel log messages are developer-facing and stay in
English.

## 12. Development workflow

Each phase is defined in `ROADMAP.md` and executed with the Standard phase prompt
in `docs/PROMPTS.md`.

Start of phase: inspect repo, read docs, review implementation, identify
dependencies, restate acceptance criteria, plan, implement the smallest useful
increment.

End of phase: compile, run tests, run static analysis, inspect warnings (they
are errors), fix regressions, update docs, record limitations, summarize
verified results.

**Git (clarified):** committing and pushing to `main` at the end of each phase is
authorized. Never force-push, never rewrite published history, never silently
discard changes, never rewrite the whole repository when a small change suffices.
Do not generate large amounts of untested code to look productive.

## 13. Testing and quality gates

- Host unit tests for architecture-independent logic (allocators, parsers,
  data structures) — built with the host compiler and sanitizers
  (`-fsanitize=address,undefined`).
- In-kernel self-tests runnable in QEMU, reporting via serial, exiting through
  `isa-debug-exit`.
- QEMU integration tests in CI with a timeout (a hang is a failure).
- Hardware tests only when hardware exists.
- Test matrix in `TESTING.md`.

Status vocabulary (use exactly): **Implemented, Compiles, Unit-tested,
Emulator-tested, Hardware-tested, Production-ready.** A passing build is not
proof of correctness or security.

## 14. Project structure

Initial proposal (create directories only when they get real content):

```
SMY-OS/
  boot/            bootloader config, image creation
  kernel/
    arch/x86_64/   CPU, GDT/IDT, paging, context switch, APIC
    core/          init, panic, logging, kernel library
    memory/        PMM, VMM, heap
    scheduler/  interrupts/  syscall/  process/  ipc/  security/
    drivers/       driver model + drivers
    storage/       block, partitions, VFS, filesystems
  system/          init, services, runtime (user space)
  compat/          see §4.1
  graphics/        display, compositor, window_manager, rendering (user space)
  desktop/         shell, launcher, settings, notifications
  design/          design tokens, icons, brand assets (asset library)
  libraries/  applications/  packages/  sdk/
  tests/           unit/ integration/ compatibility/ performance/
  tools/           host tools and scripts
  docs/            architecture/ kernel/ security/ compatibility/ design/ testing/ decisions/
```

Generated files go to `build/` (git-ignored).

## 15. Documentation

Root: `README.md`, `CONTRIBUTING.md`, `ROADMAP.md`, `STATUS.md`,
`ARCHITECTURE.md`, `SECURITY.md`, `BUILDING.md`, `TESTING.md`,
`COMPATIBILITY.md`, `DESIGN_SYSTEM.md`, `LICENSES.md`, `THREAT_MODEL.md`.

ADRs in `docs/decisions/NNNN-title.md` (Context, Decision, Alternatives,
Consequences). For each major feature: purpose, design, dependencies, status,
limitations, tests, future work. Never invent test results, benchmarks,
compatibility results or hardware support.

## 16. Licenses and originality

- Evaluate licenses of every third-party library, font, icon, firmware and tool
  before use; keep an inventory (SBOM where practical) in `LICENSES.md`.
- **Project license: OWNER DECISION.** Claude prepares a comparison (e.g.
  MIT/BSD-2 vs Apache-2.0 vs GPL-2.0/3.0 vs MPL-2.0 with trade-offs) and the owner
  chooses. Until then the repository states "All rights reserved — license
  pending".
- Do not copy proprietary code, copyrighted UI assets, private APIs or protected
  branding. Compatibility uses documented interfaces.
- List patent, trademark and redistribution questions needing legal review.
- No promise of worldwide commercial readiness because code compiles.

## 17. Initial actions

These are split across Phase 0–2 of `ROADMAP.md` (one phase per owner command):

1. Report host OS, CPU arch, toolchains, emulator, debugger, dependencies.
2. Detect existing project files; protect them.
3. Architecture proposal.
4. Repository structure and essential documentation.
5. Build system and development scripts.
6. Boot strategy (ADR).
7. First minimal original kernel loadable by the chosen boot protocol.
8. Build it and boot it in QEMU (in the session if possible, always in CI).
9. Record actual build and boot results.
10. Define the next milestone (exceptions, memory, diagnostics).

Do not create fake implementations of Windows/Linux compatibility, networking,
GPU drivers or the desktop. Create architectural plans and interfaces where
justified; implement them when dependencies are ready.

## 18. Communication

Be precise and honest. Explain important decisions simply. When several valid
solutions exist, compare trade-offs and recommend one. Decide minor technical
matters yourself. Ask before destructive or irreversible operations,
publication, purchases, external-system actions, and license choice. On a
blocker, explain it and try reasonable alternatives. Split oversized work
instead of pretending to finish it. After each phase give a short summary (in
Uzbek for the owner) and the next concrete action.

## 19. Final objective (long-term, not current claims)

A distinctive, secure, maintainable, high-performance OS with an original
kernel, independent system services, a distinctive desktop, original icons and
assets, a native app platform, filesystems and networking, modern security,
broad hardware support, Linux and Windows application compatibility, developer
tools, internationalization, accessibility, automated testing, reproducible
builds and comprehensive documentation.

Prioritize correctness, security, maintainability and measurable progress over
feature counts.
