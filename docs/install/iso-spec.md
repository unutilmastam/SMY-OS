# SMY-OS ISO build specification

Status: **Implemented and emulator-tested (Phase 2).** The ISO boots the
current SMY Kernel in QEMU on BIOS and UEFI, as CD and as raw disk. It is
**not** a live environment or an installer yet, and it has **not** been booted
on real hardware or a real USB stick. Boot architecture: [ADR 0008](../decisions/0008-boot-architecture.md).

## 1. Command

```sh
make iso      # -> build/smy-os.iso and build/SHA256SUMS
```

Inputs: the kernel ELF (`make kernel`), Limine 12.9.3 built from its
SHA-256-verified source tarball ([ADR 0007](../decisions/0007-limine-from-source.md)),
`boot/limine.conf`, `LICENSE`, `NOTICE`. Rules: `boot/boot.mk`.

## 2. Layout

| Path in ISO | Source | Purpose |
|---|---|---|
| `/boot/smy-kernel.elf` | `build/kernel/smy-kernel.elf` | SMY Kernel (ELF64, higher half) |
| `/boot/limine/limine.conf` | `boot/limine.conf` | Limine menu: one entry, timeout 0, serial on |
| `/boot/limine/limine-bios.sys` | Limine build | BIOS stage 2/3 |
| `/boot/limine/limine-bios-cd.bin` | Limine build | El Torito BIOS boot image |
| `/boot/limine/limine-uefi-cd.bin` | Limine build | FAT image with `EFI/BOOT/BOOTX64.EFI`, El Torito UEFI image and the ESP of the hybrid disk |
| `/EFI/BOOT/BOOTX64.EFI` | Limine build | UEFI loader on the ISO 9660 file system as well |
| `/boot/limine/COPYING`, `/boot/limine/3RDPARTY.md` | Limine source | Licence notices required for redistributing Limine binaries |
| `/LICENSE`, `/NOTICE` | repository | SMY-OS licence (Apache-2.0) |

Volume ID: `SMY-OS`. File systems: ISO 9660 with Rock Ridge and Joliet, plus
HFS+ (from Limine's recommended `xorriso` options).

## 3. Hybrid image (CD and USB)

`xorriso -as mkisofs` with Limine's documented options
(`third_party/downloads/limine-12.9.3/USAGE.md`), then `limine bios-install`:

| Boot path | How it works | Tested by |
|---|---|---|
| BIOS, CD | El Torito, no emulation, `limine-bios-cd.bin` | `make boot-test-bios-cd` |
| BIOS, disk / USB | MBR (protective MS-DOS label) + Limine stage 1 written by `limine bios-install` | `make boot-test-bios-disk` |
| UEFI, CD | El Torito EFI image `limine-uefi-cd.bin` | `make boot-test-uefi-cd` |
| UEFI, disk / USB | `-efi-boot-part --efi-boot-image`: the same FAT image appended as an EFI System Partition; firmware runs `\EFI\BOOT\BOOTX64.EFI` | `make boot-test-uefi-disk` |

"Disk" tests attach the ISO file as a raw IDE hard disk in QEMU, which is what a
USB stick written with `dd` looks like to firmware. A real USB stick on a real
PC is **not tested** (Phase 36).

## 4. Boot sequence

1. Firmware loads Limine (BIOS or UEFI path above).
2. Limine reads `/boot/limine/limine.conf`, loads `/boot/smy-kernel.elf` with
   the Limine protocol (base revision 6).
3. `_start` (`kernel/arch/x86_64/entry.S`) aligns the stack and calls `kmain`.
4. `kmain` (`kernel/core/main.c`): COM1 serial log → checks base revision →
   logs bootloader and firmware type → paints the boot framebuffer → logs
   `SMY-OS: boot OK` → halts.

Serial lines (stable; the boot test matches them):

```
SMY-OS: SMY Kernel starting
SMY-OS: bootloader: Limine 12.9.3
SMY-OS: firmware: BIOS | UEFI
SMY-OS: framebuffer: <W>x<H> 32bpp | none | unsupported format
SMY-OS: boot OK
```

Any line `SMY-OS: PANIC: ...` means failure.

## 5. Reproducibility

- `SOURCE_DATE_EPOCH` = time of the last commit (`mk/toolchain.mk`); exported
  to Limine's build and to `xorriso` (volume dates, volume UUID).
- Every file in the staging tree gets that timestamp before `xorriso` runs.
- Kernel and Limine are compiled with `-ffile-prefix-map` (without a trailing
  slash, so `DW_AT_comp_dir` is mapped too), so absolute paths (checkout and
  build directory) do not enter debug info or Limine's embedded build-id.
- Limine always assembles its BIOS stages with `nasm -g -F dwarf`, and NASM's
  DWARF records the absolute directory. `tools/limine-build/nasm`, a wrapper
  placed first in `PATH` only during the Limine build, drops those two flags.
- Tools are pinned: Ubuntu 24.04 packages (clang/lld 18.1.3, xorriso 1.5.6,
  mtools 4.0.43, nasm 2.16.01) and Limine 12.9.3 by SHA-256.
- Check: `tools/check-reproducible.sh` builds the ISO twice from scratch —
  once in this checkout, once in a copy of the source tree at another absolute
  path — and requires identical SHA-256. Runs in CI.

Found while implementing (Phase 2): (1) without prefix maps, Limine's BIOS
stages differed between build directories in 20-byte build-id fields;
(2) with trailing-slash maps and NASM debug info, the same commit still gave a
different ISO in CI (`/home/runner/work/...`) than in the cloud session
(`/home/user/...`). Both are fixed and the check now builds at two paths.

## 6. Release artefacts

`build/SHA256SUMS` lists the ISO hash. CI uploads `smy-os.iso` and
`SHA256SUMS` as a workflow artefact (kept 14 days). They are **test builds**,
not releases: no signature yet (Phase 35), not hardware-tested (Phase 36),
Secure Boot not supported (Phase 37) — the user must disable Secure Boot.

## 7. Limitations (Phase 2)

- The kernel only reports "boot OK" and halts; no live environment (Phase 30).
- No initramfs module yet (Phase 10).
- Boot framebuffer only: no graphics driver.
- QEMU only; no physical hardware or real USB stick tested.
