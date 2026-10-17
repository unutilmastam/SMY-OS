# Hardware assumptions

Status: planning (Phase 0). Nothing is hardware-tested.

## Initial target

| Item | Assumption | Reason |
|---|---|---|
| CPU | x86-64 (AMD64 / Intel 64), long mode, NX bit required | 64-bit only keeps the kernel simple; NX needed for W^X |
| Optional CPU features | SMEP, SMAP, `syscall`/`sysret`, APIC; used when detected | security hardening, fast syscalls |
| Firmware | UEFI (primary), legacy BIOS (secondary) via Limine | ADR 0005 |
| Machine model in tests | QEMU `q35`; OVMF for UEFI; SeaBIOS for BIOS | modern chipset, PCIe, AHCI available |
| RAM | ≥ 256 MiB for early phases (exact minimum measured later) | — |
| Display | boot framebuffer from Limine (UEFI GOP / VBE) | no GPU driver needed early |
| Input | PS/2 in QEMU first; USB HID via xHCI before any real-PC use (Phase 28) | many PCs have no PS/2 |
| Storage | virtio-blk in QEMU (Phase 14); AHCI and NVMe for real PCs (Phase 27) | — |

## Portability rule

Architecture-specific code only in `kernel/arch/x86_64/`. Architecture-
independent code never includes x86 headers, so an AArch64 or RISC-V port stays
possible.

## Real hardware

No real hardware is available to the owner. "Hardware-tested" may be used only
after Phase 36 records results in `docs/install/hardware-tested.md`.
