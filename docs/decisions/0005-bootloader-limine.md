# ADR 0005 — Bootloader: Limine (boot protocol: Limine)

- **Status:** Accepted
- **Date:** 2026-10-17
- **Phase:** 0

## Context

SMY Kernel needs a bootloader that loads a 64-bit ELF kernel on x86-64 PCs from
**UEFI** (primary) and **legacy BIOS** (secondary), from an ISO that also works
when written raw to a USB drive. The bootloader is a tool, not part of the
kernel (`CLAUDE.md` hard rules). The master prompt (§3.4) asks to evaluate
Limine first, checking licence, protocol and current stable version online.

## Evaluation (checked online on 2026-10-17, not from memory)

| Question | Finding | How checked |
|---|---|---|
| Latest release | **v12.9.3**, released 2026-10-06 (tag commit `e3aa138d383ca4fe061f05931b5a54497cd502f2`) | `git ls-remote --tags` on `github.com/limine-bootloader/limine`, highest semantic version; `ChangeLog` of that tag |
| Versioning | Semantic Versioning since 7.x | upstream `README.md` |
| Licence (Limine itself) | **BSD-2-Clause** | `COPYING` at the tag |
| Bundled third-party code | 0BSD (protocol header, freestanding headers, pdgzip), BSD-2-Clause (Flanterm, libfdt), MIT (stb_image, QOI, multiboot2.h, LAI-derived code), Apache-2.0 WITH LLVM-exception (cc-runtime), Apache-2.0 (PCG), CC0-1.0 (BLAKE2, OSDev Wiki code), PicoEFI (BSD-2/3, BSD-2-Clause-Patent, MIT) | `3RDPARTY.md` at the tag |
| Boot protocol | Limine boot protocol (spec + C header, 0BSD, separate repo `limine-protocol`) | upstream `README.md`, `3RDPARTY.md` |
| Firmware paths | UEFI (x86-64) and BIOS; hybrid ISO via `limine bios-install` | upstream `README.md`, `USAGE.md` |
| Binaries | Prebuilt binary packages are published for point releases (`limine-binary-*` release assets) | upstream `README.md` |

What the Limine protocol gives the kernel at entry: 64-bit long mode, a
higher-half kernel mapping, a higher-half direct map (HHDM) of physical memory,
the memory map, a framebuffer (UEFI GOP / VBE), RSDP pointer, loaded modules
(our initramfs), SMP start-up for application processors, and the kernel
command line. These remove a large amount of firmware-specific code from SMY
Kernel without dictating its design.

## Decision

- Use **Limine** with the **Limine boot protocol**.
- Pin **v12.9.3**. Phase 1's `tools/fetch-limine.sh` downloads the release
  binary package into `third_party/downloads/` (git-ignored) and verifies a
  **SHA-256 recorded in the script** (computed once from the official release
  asset and re-checked on each download). Upgrades are explicit commits that
  change version and hash together.
- The protocol header `limine.h` (0BSD) is vendored from the
  `limine-protocol` repository at the commit matching the pinned release, with
  its licence noted in `LICENSES.md`.
- Limine's licence notices (`COPYING`, `3RDPARTY.md`) are shipped inside every
  ISO under `/boot/limine/` to satisfy the BSD/MIT/Apache notice requirements of
  its binary redistribution.

## Alternatives

| Option | Advantages | Disadvantages | Verdict |
|---|---|---|---|
| **Limine** | BSD licence; UEFI + BIOS + hybrid ISO from one tool; protocol gives long mode, HHDM, memmap, framebuffer, modules, SMP; actively maintained | We depend on its release cadence; not Microsoft-signed (Secure Boot needs work, Phase 37) | **Chosen** |
| GRUB 2 + Multiboot2 | Very widespread | GPL-3.0; Multiboot2 enters in 32-bit protected mode → we write long-mode setup ourselves; heavier ISO tooling | Rejected |
| Own UEFI loader | Full control, no external dependency | Large amount of work before the first kernel line runs; BIOS path would need a second loader | Rejected for now (possible later) |
| BOOTBOOT | Simple protocol | Smaller project, less active | Rejected |
| systemd-boot / rEFInd | UEFI only | Not kernel loaders for our protocol; no BIOS | Rejected |

## Consequences

- Phase 2 can boot a C kernel in QEMU quickly for both UEFI (OVMF) and BIOS.
- SMY Kernel must not depend on bootloader page tables long-term: Phase 5
  replaces them with its own.
- Secure Boot: Limine is not signed by Microsoft. Until Phase 37, users must
  disable Secure Boot — stated plainly in docs (`docs/INSTALLER_PROMPT.md` §15).
- Revisit if Limine's licence changes, the project becomes unmaintained, or a
  protocol change breaks us across a major version.
