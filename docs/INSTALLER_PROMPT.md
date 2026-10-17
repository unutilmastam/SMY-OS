# SMY-OS — Installation and Deployment Prompt (revised v1.1)

> The owner's original installer prompt, reviewed and corrected. Corrections are
> listed in `docs/PROMPT_REVIEW.md` (section "Installer prompt"). Executed in the
> phases of `ROADMAP.md` marked track **Install**. Where this file and the
> original disagree, this file wins.

## Goal

Let end users install SMY-OS on compatible x86-64 PCs from a bootable USB
drive, through an original installer that follows the SMY design system.
Build it incrementally. Installation is **not supported** until every
component below is implemented and tested; `STATUS.md` must say so.

## Architecture (added)

```
boot/                ISO layout, Limine config, image scripts
system/live/         live-environment services (user space)
system/installer/
  engine/            installer engine: plan → confirm → execute → verify
  frontend-text/     text front-end (recovery, scripted tests)
  frontend-gui/      graphical front-end (SMY Desktop toolkit)
  hwcheck/           hardware compatibility checks
system/firstboot/    first-boot setup
tools/release/       release pipeline (ISO, checksums, signatures)
tests/install/       QEMU installation scenarios
docs/install/        ISO spec, boot architecture, user guide, troubleshooting
```

- The **installer engine** is a user-space program, separate from the kernel
  and from any UI. Both front-ends only build a plan and show it; the engine
  executes it. This allows fully scripted QEMU tests with the same code path.
- The engine works on an explicit **plan** (a data structure listing every
  operation: create GPT, create partition X of size Y, format, copy, install
  bootloader, write boot entry). The plan is shown in full before execution.
- Every destructive operation needs a confirmation bound to that exact plan
  and that exact disk identity (model + serial + size). If anything changes,
  confirmation is void.

## Requirements

1. **Reproducible ISO pipeline.** One command (`make iso`) assembles kernel,
   initramfs (system files, drivers, services, and later the graphical
   environment), and boot configuration into an ISO. Reproducible:
   `SOURCE_DATE_EPOCH`, fixed file order and timestamps, pinned tool versions;
   CI builds twice and compares SHA-256. The ISO contains only what exists —
   it grows phase by phase.
2. **Boot paths.** Documented x86-64 **UEFI** path (primary). Legacy BIOS
   through Limine as a secondary path, tested in CI but lower priority.
   The ISO must be a **hybrid image**: it boots from optical media and when
   written raw to a USB drive (`dd`, balenaEtcher, Rufus "DD mode").
3. **Live environment.** Boots entirely from memory (root filesystem in the
   initramfs loaded by the bootloader), so no USB driver is needed to run it.
   In live mode, internal disks are opened **read-only** by policy enforced in
   the storage layer, not just in the UI. A test verifies that a live boot does
   not change a virtual disk's SHA-256.
4. **Original graphical installer** consistent with `DESIGN_SYSTEM.md`. Built
   only after the UI toolkit and desktop exist. The text front-end comes
   first.
5. **Pre-install checks.** Hardware compatibility (x86-64 with required CPU
   features, NX; RAM minimum; UEFI vs BIOS mode; supported storage
   controller; disk size minimum; supported keyboard), prerequisites
   (power on laptops when measurable), language selection, keyboard layout
   (incl. Uzbek Latin/Cyrillic), storage discovery with model, serial, size,
   partition table, partitions, filesystems and labels.
6. **Options**, enabled only when their capability is verified:
   - Live environment without installation.
   - Install to a dedicated (whole) disk.
   - Install to explicitly selected **existing unallocated space**.
   - Dual-boot alongside an existing OS — only into unallocated space the user
     created beforehand (e.g. with Windows Disk Management). **SMY-OS does not
     resize NTFS, APFS or other foreign filesystems.** Detect and warn about
     BitLocker and Windows Fast Startup / hibernation.
7. **Safe partitioning subsystem.** Shows the physical disk, capacity,
   existing partitions and all destructive operations. GPT only for new
   layouts (MBR disks: whole-disk install only, after confirmation). Writes
   both primary and backup GPT, validates CRCs, aligns to 1 MiB.
8. **Explicit confirmation** before formatting, repartitioning or overwriting
   — the user types or selects the disk name shown, not just "Next".
9. **Never** format silently, never pick the first disk by default, never
   pre-select the disk holding the live medium.
10. **Errors, logs, interruptions, recovery.** Structured install log
    (timestamps, plan, each step, result); log saved to the target disk and
    offered for saving to the ESP. A step journal lets the engine detect an
    interrupted install on next boot and offer a clean retry. Power loss must
    never leave a state that the firmware boots into a half-installed system:
    the boot entry is written **last**.
11. **Install layout.** Kernel, system files and configuration go to documented
    paths on the SMY root filesystem; boot files to the EFI System Partition
    under `\EFI\SMY-OS\`.
12. **Boot configuration.** Install Limine to `\EFI\SMY-OS\`. Create a UEFI
    boot entry via UEFI runtime `SetVariable` (requires kernel support for
    runtime services). The removable fallback path `\EFI\BOOT\BOOTX64.EFI` is
    written **only** on a disk dedicated to SMY-OS; on a shared ESP it is never
    overwritten. Provide a recovery boot entry and a documented manual recovery
    procedure from the live USB.
13. **Verification before success.** SHA-256 manifest of every installed file,
    re-read and compared after copying; GPT and filesystem consistency check;
    boot entry read back. Only then report success.
14. **First-boot setup.** Account creation (password hashed with an
    established algorithm, e.g. Argon2id), language, keyboard, time zone,
    privacy and security defaults (telemetry: none by default).
15. **Secure Boot.** Document requirements: Limine is not Microsoft-signed;
    options are (a) user disables Secure Boot, (b) user enrolls SMY-OS keys,
    (c) a signed shim (long external review process). Implement and test
    signing + verification before any Secure Boot claim. Until then the
    installer tells the user plainly that Secure Boot must be disabled.
16. **QEMU first.** All installer tests use disposable `qcow2`/raw images
    created per test, with OVMF (UEFI) and separate NVRAM vars files.
17. **Scenarios:** fresh install (whole disk), install into free space next to
    a pre-made "foreign" partition (checked unchanged by hash), failed install
    (injected write error), interrupted install (QEMU killed mid-copy), reboot
    into installed system, recovery from live USB, dual-boot entry ordering.
18. **Separation.** Live environment and installer are user-space services;
    kernel changes are limited to drivers, read-only policy, and UEFI runtime
    services.
19. **Deliverables.** Build scripts, install logs, troubleshooting docs,
    USB-writing guide, `SHA256SUMS` (signed once package signing exists),
    release notes listing tested hardware and known limitations.
20. **Data safety.** Automated tests never touch real disks (the test runner
    refuses any block device path; only image files). Destructive operations on
    physical hardware require explicit authorization by the person at that
    machine.

## Real-hardware prerequisites (added)

QEMU's virtio devices are not on real PCs. Before any physical install:

- Storage drivers: **AHCI (SATA)** and **NVMe**.
- Input: **xHCI + USB HID keyboard/mouse** (many PCs have no PS/2).
- Display: UEFI GOP framebuffer (already via Limine) — enough for the installer.
- Writable filesystems: GPT write, **FAT32 write** (ESP), SMY root filesystem
  write + check tool.
- UEFI runtime services (boot entries, time).
- A physical test machine and a PC to write the USB. The owner has no
  computer, so physical testing needs a borrowed/test PC and is marked
  **OWNER DECISION**.

## Initial milestone (unchanged in spirit)

1. ISO build specification (`docs/install/iso-spec.md`) and boot architecture
   ADR.
2. Smallest bootable hybrid ISO with the current kernel.
3. Boot it in QEMU (UEFI and BIOS), record actual results.

This is merged into **Phase 2** of `ROADMAP.md`. Live environment, installer,
partitioning and full workflow come only after it succeeds and their
dependencies exist.
