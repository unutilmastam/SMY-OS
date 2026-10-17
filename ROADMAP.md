# SMY-OS Roadmap

Statuses: `NOT STARTED` · `IN PROGRESS` · `BLOCKED` · `VERIFIED`.
A phase is `VERIFIED` only when **all** its acceptance criteria pass in GitHub
Actions CI (and locally when the tools exist) and docs are updated.
One phase per owner command ("davom et"). Each phase ends with commit + push to
`main`.

Phases marked **OWNER DECISION** stop and ask the owner before the decision
point, then continue in the next command.

| # | Phase | Track | Status |
|---|---|---|---|
| 0 | Foundation: environment report, docs skeleton, ADRs | Project | NOT STARTED |
| 1 | Build system, toolchain, CI, host test framework | Build | NOT STARTED |
| 2 | First boot: Limine + minimal kernel + QEMU boot test | Kernel | NOT STARTED |
| 3 | CPU: GDT/TSS, IDT, exceptions, panic, kernel log | Kernel | NOT STARTED |
| 4 | Physical memory manager | Kernel | NOT STARTED |
| 5 | Virtual memory + kernel heap | Kernel | NOT STARTED |
| 6 | D1 — Design System specification + tokens | Design | NOT STARTED |
| 7 | Interrupt controllers, timers, keyboard input | Kernel | NOT STARTED |
| 8 | Threads, scheduler, synchronization | Kernel | NOT STARTED |
| 9 | C0+C1 — Compatibility architecture + ELF/PE inspector | Compat | NOT STARTED |
| 10 | User mode, syscalls, initramfs, first native program (C2) | Kernel | NOT STARTED |
| 11 | Processes, ELF loader, IPC | Kernel | NOT STARTED |
| 12 | D2 — SMY icon system | Design | NOT STARTED |
| 13 | VFS, device model, framebuffer text console | Kernel | NOT STARTED |
| 14 | PCI, virtio-blk, GPT, FAT32 (read) | Storage | NOT STARTED |
| 15 | C3 — Linux personality: first static Linux binary | Compat | NOT STARTED |
| 16 | SMP (multi-core) | Kernel | NOT STARTED |
| 17 | D3 — Component specifications | Design | NOT STARTED |
| 18 | SMY Graphics: user-space display server v0 | Graphics | NOT STARTED |
| 19 | Native UI toolkit primitives (from tokens) | Graphics | NOT STARTED |
| 20 | SMY Desktop v0: shell, window manager, launcher | Desktop | NOT STARTED |
| 21 | First apps: terminal, file manager, settings, system monitor | Apps | NOT STARTED |
| 22 | Networking 1: virtio-net, Ethernet, ARP, IPv4, ICMP | Network | NOT STARTED |
| 23 | Networking 2: UDP, DHCP, DNS, TCP | Network | NOT STARTED |
| 24 | C4 — First Windows PE console program | Compat | NOT STARTED |
| 25 | Security: users, auth, permissions, sandbox, audit | Security | NOT STARTED |
| 26 | SMY Package System v0 | Packages | NOT STARTED |
| 27 | Writable storage, installer, login + lock screen | System | NOT STARTED |
| 28 | Planning phase: define phases 29+ | Project | NOT STARTED |

---

## Phase 0 — Foundation

**Goal:** a documented project skeleton and the key early decisions.

Scope:
- Environment report (host OS, CPU, clang/lld/gcc/nasm/qemu/gdb/make/cmake/
  xorriso versions; what is missing; what CI runners provide) →
  `docs/architecture/environment.md`.
- Root docs from `docs/MASTER_PROMPT.md` §15 with real content (no lorem ipsum):
  README, CONTRIBUTING, STATUS, ARCHITECTURE (architecture proposal), SECURITY,
  BUILDING, TESTING (test matrix), COMPATIBILITY (C0–C8 table), DESIGN_SYSTEM
  (pointer to D1), LICENSES (inventory + license comparison), THREAT_MODEL (v0).
- ADRs: 0001 kernel architecture (modular monolith), 0002 languages (C17 +
  asm syntax choice; Rust deferred), 0003 toolchain (clang + lld), 0004 build
  system, 0005 bootloader (Limine evaluation: licence, protocol, current stable
  version — checked online, not from memory).
- **OWNER DECISION:** project licence — present a short comparison table in the
  Uzbek report and ask. Until answered, `LICENSES.md` says "license pending".

Acceptance: all files exist with real content; ADRs follow the template;
markdown link check passes (a simple script is fine).
Out of scope: any code.

## Phase 1 — Build system, toolchain, CI

**Goal:** `make` (or chosen system) builds an empty kernel object and host tests;
CI runs on every push.

Scope: build system per ADR 0004; strict warning flags; `clang-format` config;
`tools/check-env.sh` (verifies required tools and versions);
`tools/fetch-limine.sh` (pinned version + SHA-256, downloads into
`third_party/downloads/`); host unit-test harness (minimal, in-repo, or a small
licensed framework with reason); one real unit test (e.g. kernel `string`
functions: `memcpy`, `memset`, `strlen`) built with ASan/UBSan;
`.github/workflows/ci.yml`: install tools via apt, build, run unit tests,
format check, static analysis (`cppcheck` or `clang-tidy`).

Acceptance: CI green; a deliberately failing unit test makes CI red (verify once
locally, do not commit it).

## Phase 2 — First boot

**Goal:** the first original SMY Kernel boots in QEMU.

Scope: Limine config; linker script (higher-half kernel); `_start` in C/asm;
COM1 serial driver (16550) for logging; draw a solid colour + SMY-OS boot mark
via the Limine framebuffer; bootable ISO creation (xorriso + Limine);
`tools/run-qemu.sh` (headless, `-serial stdio`, `isa-debug-exit`);
`tests/integration/boot_test.sh`: boots, waits for `SMY-OS: boot OK` on serial,
timeout 30 s; CI job running it for **BIOS and UEFI (OVMF)**.

Acceptance: boot test passes in CI for both firmware modes; build result and
boot log excerpt recorded in `STATUS.md`.

## Phase 3 — CPU foundation

Scope: GDT + TSS (with IST stacks for double fault/NMI), IDT, exception
handlers for vectors 0–31 printing registers and error code; `panic()` with
message and stack trace (frame pointers); kernel logger with levels;
in-kernel self-test framework (`ktest`) run with a boot flag; tests that
trigger `#UD`, `#BP` and `#PF` deliberately and assert they are caught.

Acceptance: QEMU self-test run passes in CI; exception output documented.

## Phase 4 — Physical memory manager

Scope: parse Limine memory map; bitmap or buddy allocator (ADR); page-frame
alloc/free; statistics; host unit tests of the allocator core (with a fake
memory map, including fragmentation and exhaustion cases) + in-kernel test.

Acceptance: host tests under ASan/UBSan pass; in-kernel test allocates/frees
all usable memory without leaks.

## Phase 5 — Virtual memory + heap

Scope: own 4-level page tables (replace bootloader tables), HHDM, map/unmap API,
NX and W^X for kernel sections, SMEP/SMAP when supported, page-fault diagnostics,
kernel heap (`kmalloc/kfree`, slab or size classes), guard pages for kernel
stacks.

Acceptance: host tests for heap logic; QEMU tests: map/unmap, write to RO page
faults, heap stress test.

## Phase 6 — D1 Design System specification

Execute `docs/DESIGN_PROMPT.md` steps 1, 2, 3, 5, 9, 10, 13 (spec only):
concept + principles, colour system for 3 themes, foundations tokens in
`design/tokens/*.json`, generator (`tools/design/gen_tokens.py` → C header +
CSS), automated WCAG contrast test in CI, `DESIGN_SYSTEM.md` full rationale,
font licence evaluation (OWNER is informed of the chosen OFL font).

Acceptance: CI runs generator + contrast tests; generated files not committed
(or committed and verified identical — choose and document).
Out of scope: desktop implementation.

## Phase 7 — Interrupts, timers, input

Scope: ACPI table discovery (RSDP from Limine, MADT), disable legacy PIC,
Local APIC + I/O APIC, timer (LAPIC calibrated with HPET or PIT), monotonic
clock API, PS/2 keyboard driver producing key events into a ring buffer (with
scancode set → keycode mapping, layout independent).

Acceptance: QEMU test: timer ticks measured; keystrokes injected via QEMU
monitor (`sendkey`) are received.

## Phase 8 — Threads and scheduler

Scope: kernel threads, context switch (asm), preemptive round-robin scheduler
with priorities (ADR), sleep/wakeup, spinlocks, mutexes, semaphores, wait
queues; scheduler benchmark baseline.

Acceptance: QEMU tests for preemption, mutex correctness under contention,
sleep accuracy.

## Phase 9 — C0 + C1 Compatibility foundation

Scope: `COMPATIBILITY.md` complete C0 research (Linux personality design,
Windows design, API/ABI/translation/emulation differences, licence notes on
Wine/ReactOS); `compat/executable_loader/` ELF64 and PE32+ parsers usable
from host and kernel; host tool `smy-inspect`; unit tests with valid samples
and malformed inputs; simple mutation fuzz test (10 000 inputs, no crash under
ASan).

Acceptance: tests + fuzz pass in CI.

## Phase 10 — User mode (C2)

Scope: initramfs as USTAR tar loaded as a Limine module; ring-3 entry, per-
process address space, `syscall/sysret` with validated arguments
(copy_from_user), minimal native syscalls (`write` to console, `exit`,
`yield`); first native SMY program `init` built for user space.

Acceptance: QEMU test: `init` prints a message from ring 3 and exits; a test
program attempting to read kernel memory is killed, not the kernel.

## Phase 11 — Processes, ELF loader, IPC

Scope: process objects, spawn/exit/wait, ELF loader using Phase 9 parser
(static ELF), handle/capability table, IPC channels (message passing) designed
for the display server and services.

Acceptance: QEMU tests: parent spawns children, waits exit codes; IPC ping-pong
benchmark baseline.

## Phase 12 — D2 Icon system

Execute `docs/DESIGN_PROMPT.md` step 4, 11, 12: grid/keylines spec, required
icon set as original SVG in `design/icons/`, manifest with status, icon lint
(viewBox, stroke width, no fills outside rules, no raster), host-rendered
reference sheet PNG (via a licensed renderer, reason recorded).

Acceptance: all required icons present with `status: final`; lint passes in CI.

## Phase 13 — VFS and devices

Scope: VFS (vnodes, mount table, path lookup), initramfs as mounted FS, file
descriptor table, `open/read/write/close/seek/stat`, device model (driver
registration), framebuffer text console with a licensed bitmap font, UTF-8
output.

Acceptance: QEMU tests read files from initramfs through syscalls.

## Phase 14 — Storage

Scope: PCI enumeration, virtio-blk driver, block cache, GPT parser, FAT32
read-only driver; disk images built in CI only (never real disks).

Acceptance: QEMU test reads a known file from a FAT32 partition on a virtual
disk; host tests for GPT/FAT parsing with corrupted images.

## Phase 15 — C3 Linux personality

Scope: per-process personality flag; Linux x86-64 syscall table subset from
`docs/MASTER_PROMPT.md` §4.2; musl static test program built in CI;
`docs/compatibility/linux-syscalls.md` table.

Acceptance: QEMU test runs the musl binary, checks output and exit code.

## Phase 16 — SMP

Scope: AP startup (via Limine MP), per-CPU data, SMP-safe scheduler and locks,
TLB shootdown.

Acceptance: QEMU `-smp 4` stress tests pass repeatedly (10 runs in CI).

## Phase 17 — D3 Component specifications

Execute `docs/DESIGN_PROMPT.md` steps 6, 7, 8, 13: every component specified
(anatomy, states, keyboard model, tokens, accessibility roles) +
implementation plan mapped to phases 18–27; token-literal lint.

## Phase 18 — SMY Graphics display server v0

Scope: framebuffer mapping to a privileged-but-user-space display service,
input event delivery via IPC, surface/buffer protocol (ADR), software
compositor with damage tracking, frame-time measurement.

Acceptance: QEMU screenshot test (QEMU `screendump`) compared against a
reference image; frame-time numbers recorded.

## Phase 19 — Native UI toolkit

Scope: drawing primitives, text rendering (licensed font rasterizer, reason +
licence), layout, focus management, token-driven theming (light/dark/HC),
scale factors.

Acceptance: screenshot tests per theme and scale factor.

## Phase 20 — SMY Desktop v0

Scope: desktop shell, window manager (move, resize, focus, workspaces,
keyboard control), application launcher, per D3 specs.

Acceptance: scripted keyboard tests via QEMU `sendkey` + screenshot checks.

## Phase 21 — First native apps

Scope: terminal, file manager (read-only to start), settings (theme, scale,
keyboard layout), system monitor; localization catalogue (English, Uzbek).

## Phase 22–23 — Networking

As in `docs/MASTER_PROMPT.md` §8, tested with QEMU user networking
(ping gateway, DHCP lease, DNS query, TCP connection to a CI-hosted server).

## Phase 24 — C4 Windows PE

Scope: PE loader, minimal NT-like process services, SMY-written `kernel32`
subset for the MinGW-w64 test program (§4.3). No Wine/ReactOS code.

Acceptance: QEMU test runs the PE, checks output and exit code.

## Phase 25 — Security

Users/groups, authentication (established password-hash library), permission
checks on VFS, capability-based sandbox for apps, audit log, threat-model update.

## Phase 26 — Package System v0

Manifest format, dependency resolver (host-tested), Ed25519 signatures via an
established library, local repository, install/remove/rollback.

## Phase 27 — Writable storage, installer, login

Writable FS (ADR: FAT32 write vs ext2 vs custom), installer to a virtual disk,
login and lock screen.

## Phase 28 — Planning

Review everything, update STATUS, define phases 29+ (USB, audio, virtio-gpu,
IPv6/TLS, C5–C8, real-hardware bring-up, more apps).
