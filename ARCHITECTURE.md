# SMY-OS Architecture (proposal v0)

Status: **proposal** — nothing below is implemented yet. Each part becomes real
in the `ROADMAP.md` phase named next to it. Decisions are recorded as ADRs in
`docs/decisions/`.

## 1. Overview

```
 ┌──────────────────────────── user space (ring 3) ─────────────────────────────┐
 │  SMY Apps        SMY Desktop (shell, WM, launcher)     installer / live svcs │
 │      │                   │                                     │             │
 │  SMY Graphics display server + compositor        SMY Package System          │
 │      │                   │                                     │             │
 │  runtime libs (native)   Linux programs (musl)   Windows PE programs         │
 │                           │                       │  SMY kernel32/ntdll      │
 ├────────── native syscalls ┴── Linux personality ──┴── NT-like personality ───┤
 │                         SMY Kernel (ring 0, modular monolith)                │
 │  syscall layer · process/threads · scheduler · IPC · security/capabilities   │
 │  VFS · block layer · partitions · filesystems · network stack                │
 │  memory (PMM · VMM · heap) · interrupts · timers · driver model + drivers    │
 │  arch/x86_64 (GDT/IDT/TSS, paging, APIC, context switch, syscall entry)      │
 ├──────────────────────────────────────────────────────────────────────────────┤
 │  Limine bootloader (tool)  ·  UEFI / BIOS firmware  ·  QEMU or x86-64 PC     │
 └──────────────────────────────────────────────────────────────────────────────┘
```

Principles:

1. **Original kernel** written in this repository (ADR 0001: modular monolith).
2. **Only what needs ring 0 lives in the kernel.** Graphics, desktop, installer
   and package management are user-space services.
3. **Testable by construction.** Portable kernel logic compiles as a host
   library for unit tests; kernel behaviour is tested in QEMU in CI.
4. **Security from the first line**: W^X, NX, SMEP/SMAP, stack protector,
   validated syscall arguments, no kernel addresses leaked (`THREAT_MODEL.md`).
5. **Compatibility as personalities**, not as the base: the native SMY ABI is
   primary; Linux and Windows support is layered on top (`COMPATIBILITY.md`).

## 2. Boot path (Phases 2, 32)

Firmware (UEFI primary, BIOS secondary) → Limine (ADR 0005) → SMY Kernel ELF
entered in 64-bit long mode, higher half, with memory map, HHDM, framebuffer,
RSDP, modules (initramfs) and kernel command line → `kmain`.

The ISO is a hybrid image (CD and raw USB). Details in `docs/install/iso-spec.md`
(Phase 2) and `docs/INSTALLER_PROMPT.md`.

## 3. Kernel subsystems

| Subsystem | Directory | Responsibility | Phase |
|---|---|---|---|
| Arch layer | `kernel/arch/x86_64/` | GDT/TSS, IDT, exception stubs, paging primitives, APIC, context switch, `syscall` entry | 2, 3, 5, 7, 8, 10 |
| Core | `kernel/core/` | init sequence, `panic`, logger, kernel library (`string`, formatting), `ktest` | 2, 3 |
| Physical memory | `kernel/memory/` | page-frame allocator over the Limine memory map | 4 |
| Virtual memory | `kernel/memory/` | own page tables, HHDM, map/unmap, address spaces, heap | 5, 10 |
| Interrupts and time | `kernel/interrupts/` | ACPI MADT, LAPIC/IOAPIC, timers, monotonic clock | 7 |
| Scheduler | `kernel/scheduler/` | threads, preemptive priority round-robin, sync primitives | 8, 16 |
| Syscalls | `kernel/syscall/` | native table, argument validation, personality dispatch | 10, 15, 24 |
| Processes | `kernel/process/` | process objects, ELF loading, handle/capability table | 11 |
| IPC | `kernel/ipc/` | message channels used by services and the display server | 11 |
| Security | `kernel/security/` | users, permissions, capabilities, audit | 25 |
| Drivers | `kernel/drivers/` | driver model; serial, PS/2, PCI, virtio, AHCI, NVMe, xHCI | 2, 7, 13, 14, 27, 28 |
| Storage | `kernel/storage/` | block cache, GPT, VFS, USTAR initramfs, FAT32, root FS | 10, 13, 14, 29 |
| Network | `kernel/net/` | Ethernet → ARP → IPv4 → ICMP/UDP/TCP, DHCP/DNS clients | 22, 23 |

Module rule: a subsystem exposes one public header; others include only that.
Architecture-independent code never includes `arch/x86_64` headers.

## 4. Memory layout (planned, x86-64, 4-level paging)

| Range | Use |
|---|---|
| `0x0000_0000_0000_0000` – `0x0000_7FFF_FFFF_FFFF` | user space (per process); page 0 never mapped |
| `0xFFFF_8000_0000_0000` – … | HHDM (direct map of physical memory, offset from Limine) |
| dedicated region below the kernel image | kernel heap, kernel stacks with guard pages, MMIO mappings |
| `0xFFFF_FFFF_8000_0000` – top | kernel image (`-mcmodel=kernel`) |

Exact addresses are fixed in Phase 5 and documented in
`docs/kernel/memory-layout.md`.

## 5. User space and system services

- Native executables: ELF64 with an SMY ABI note (Phase 10/11).
- `init` (first process) → service manager → display server → desktop shell.
- IPC channels with capabilities for every service connection (Phase 11).
- Apps get no ambient authority: file, network and device access via
  capabilities granted by manifest and user consent (Phase 25).

## 6. Graphics (Phases 18–20)

User-space display server owns the framebuffer mapping and input devices,
composites client surfaces in software with damage tracking, and talks to
clients over IPC. The toolkit and desktop are generated from design tokens
(`DESIGN_SYSTEM.md`). GPU acceleration (virtio-gpu first) only after the driver
model is mature.

## 7. Storage and filesystems

Block device → GPT → filesystem driver → VFS → file descriptors. Order:
USTAR initramfs (Phase 10) → FAT32 read (14) → FAT32 write + SMY root FS
(29, ADR). Tests use disposable image files only, never real disks.

## 8. Compatibility (Phases 9, 15, 24)

Per-process personality: native, Linux (syscall table subset), Windows
(PE loader + NT-like services + SMY-written `kernel32` subset). Shared ELF/PE
parsers in `compat/executable_loader/`, host-tested and fuzzed. See
`COMPATIBILITY.md`.

## 9. Repository layout

As in `docs/MASTER_PROMPT.md` §14. Directories are created only when they get
real content. Generated output goes to `build/`.

## 10. Open questions

- Root filesystem format (existing documented format vs custom) — ADR in
  Phase 29.
- Physical memory allocator: bitmap vs buddy — ADR in Phase 4.
- Scheduler priority model — ADR in Phase 8.
- Surface/buffer protocol for the display server — ADR in Phase 18.
- Project licence — **owner decision** pending (`LICENSES.md`).
