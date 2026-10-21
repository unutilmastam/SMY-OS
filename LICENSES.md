# Licences

## 1. SMY-OS project licence

**Apache License 2.0** (SPDX `Apache-2.0`) — chosen by the owner on
2026-10-17, recorded in [ADR 0006](docs/decisions/0006-project-licence.md).
Full text: [`LICENSE`](LICENSE); notice: [`NOTICE`](NOTICE).
Copyright line: `Copyright 2026 The SMY-OS Authors`.

Options that were presented to the owner (summary; full comparison in §4):

| Option | Short description |
|---|---|
| MIT | Very permissive; anyone may reuse, including in closed products |
| Apache-2.0 | Permissive + explicit patent grant and patent-retaliation clause |
| GPL-3.0-only | Copyleft: modified versions that are distributed must stay open source |
| MPL-2.0 | File-level copyleft: changed SMY-OS files stay open, other code may be closed |

Every source file carries `SPDX-License-Identifier: Apache-2.0` and the
copyright line. Note: GPL-2.0-only code is incompatible with Apache-2.0 and
can never be included.

## 2. Third-party inventory

Status values: **in use** (in the repository or built into artefacts),
**tool** (used to build/test, not redistributed), **planned** (decided, not yet
added).

| Component | Version | Licence | Status | Use | Redistributed? | Notes |
|---|---|---|---|---|---|---|
| Limine bootloader | 12.9.3 | BSD-2-Clause (+ bundled 0BSD, MIT, BSD-2/3, Apache-2.0, Apache-2.0 WITH LLVM-exception, CC0-1.0; see upstream `3RDPARTY.md`) | in use (Phase 2): built from source | bootloader in the ISO | **yes** (binaries built from source, in the ISO) | Source tarball `limine-12.9.3.tar.xz`, SHA-256 `95f456a2…3dd4e995` pinned in `tools/fetch-limine.sh`. Ship `COPYING` + `3RDPARTY.md` in the ISO. ADR 0005, ADR 0007 |
| Limine protocol header `limine.h` | from the 12.9.3 tarball | 0BSD | in use (Phase 2) | kernel ↔ bootloader interface | yes (compiled in) | Vendored unmodified in `third_party/limine-protocol/` with its LICENSE and SHA-256 |
| LLVM / Clang / lld | 18.1.3 (Ubuntu) | Apache-2.0 WITH LLVM-exception | tool | compiler, linker, format, tidy | no | Compiler runtime is not linked into the kernel |
| compiler-rt sanitizers (`libclang-rt-18-dev`) | 18.1.3 (Ubuntu) | Apache-2.0 WITH LLVM-exception | tool | ASan/UBSan runtime for host unit tests | no | Linked only into host test binaries, never into the kernel |
| NASM | 2.16.01 (Ubuntu) | BSD-2-Clause | tool | needed to build Limine from source (Phase 2) | no | Not used for SMY-OS sources (ADR 0002) |
| curl, GNU tar, xz-utils | Ubuntu 24.04 | curl licence (MIT-style), GPL-3.0-or-later, 0BSD/public domain | tool | download and unpack pinned sources | no | |
| GNU Make | 4.3 | GPL-3.0-or-later | tool | build | no | Using a GPL tool does not affect our licence |
| QEMU | 8.2.2 (Ubuntu) | GPL-2.0 (parts other licences) | tool | emulator for tests | no | |
| OVMF (EDK II) | 2024.02 (Ubuntu) | BSD-2-Clause-Patent | tool | UEFI firmware in tests | no | |
| xorriso | 1.5.6 | GPL-3.0-or-later | tool | ISO creation | no | Output ISO is not a derivative of xorriso |
| mtools | 4.0.43 | GPL-3.0-or-later | tool | FAT images | no | |
| cppcheck | 2.13.0 | GPL-3.0-or-later | tool | static analysis | no | |
| librsvg (`rsvg-convert`) | 2.58.0 (Ubuntu) | LGPL-2.1-or-later | tool | renders SVG design assets to PNG (previews, exports, kernel boot logo pixels) | no | Output images are our own artwork |
| ImageMagick (`montage`, `convert`) | 6.9.12 (Ubuntu) | ImageMagick License | tool | composes preview sheets; converts the boot logo and QEMU screendumps to raw RGB/PNG | no | |
| DejaVu fonts (`fonts-dejavu-core`) | 2.37 (Ubuntu) | Bitstream Vera licence + public domain changes | tool | label text on generated preview sheets only | no | Never part of a logo, the ISO or any committed asset |
| Inter (font) | 4.1 | SIL OFL 1.1 | in use (Phase 6): design checks and preview sheets | UI typeface (DESIGN_SYSTEM.md §6) | not yet (ISO from Phase 20, with `LICENSE.txt`) | Release archive `Inter-4.1.zip`, SHA-256 `9883fdd4…5756b11e` pinned in `tools/fetch-fonts.sh`; not committed; no reserved font name |
| IBM Plex Mono (font) | 2.5.0 | SIL OFL 1.1, Reserved Font Name "Plex" | in use (Phase 6): design checks | monospace typeface | not yet (ISO from Phase 20, with `LICENSE.txt`) | Release archive `ibm-plex-mono.zip` (tag `@ibm/plex-mono@2.5.0`), SHA-256 `6d23f012…bbc481c` pinned in `tools/fetch-fonts.sh`; a modified (e.g. subset) version must not be named "Plex" |
| unzip | Ubuntu 24.04 | Info-ZIP licence | tool | unpack the pinned font archives | no | |
| GDB | 15.1 | GPL-3.0-or-later | tool | debugging | no | |
| Python 3 | 3.x | PSF-2.0 | tool | host scripts | no | Standard library only |
| `actions/upload-artifact` (GitHub Action) | v7.0.2 | MIT | tool | CI uploads the test ISO | no | Pinned by commit SHA `cf430e030ddbb5b0abf93d22962f4752f3646cd9` (Phase 2) |
| SeaBIOS (inside QEMU) | Ubuntu 24.04 QEMU package | LGPL-3.0 | tool | BIOS firmware in boot tests | no | |
| `actions/checkout` (GitHub Action) | v7.0.1 | MIT | tool | CI checkout | no | Pinned by commit SHA `3d3c42e5aac5ba805825da76410c181273ba90b1` (Phase 1) |

Every new dependency needs: a one-line reason, its licence, and for binaries a
pinned version + SHA-256 (`CLAUDE.md`).

## 3. Never copied (policy)

- Linux, BSD, Windows or other kernel code as a foundation.
- **Wine and ReactOS code** (LGPL-2.1 / GPL-2.0): no copying; only public
  documentation (Microsoft PE/COFF spec, API docs). Any exception needs an owner
  decision recorded here.
- Proprietary fonts (Segoe UI, San Francisco, …), icons, wallpapers or
  branding of other operating systems.
- Fonts: only SIL Open Font License families or original work (Phase 6).

## 4. Project licence comparison (for the owner decision)

| Criterion | MIT | Apache-2.0 | GPL-3.0-only | MPL-2.0 |
|---|---|---|---|---|
| Others may use in closed-source products | yes | yes | no (if they distribute) | yes, but changed MPL files stay open |
| Modified versions must publish source | no | no | **yes** (whole work) | only the changed MPL files |
| Explicit patent licence from contributors | no | **yes** | yes | yes |
| Compatible with BSD/MIT/Apache parts we use (Limine) | yes | yes | yes | yes |
| Can later include GPL-2.0-only code (e.g. a GPLv2 driver) | yes (combined work becomes GPL-2.0) | **no** | **no** | yes (via MPL §3.3 secondary-licence clause) |
| Can later include GPL-3.0 code | yes (combined work becomes GPL-3.0) | yes (combined work becomes GPL-3.0) | yes | yes (via secondary licence) |
| Company/contributor adoption | highest | high (firms prefer the patent grant) | lower for firms | medium |
| Protects against a closed fork of SMY-OS | no | no | **yes** | partly |
| Text length / simplicity | very short | long | long | medium |

Recommendation for the owner: **Apache-2.0** — permissive like MIT, but with an
explicit patent grant, which matters for an OS that implements compatibility
with commercial platforms.

## 5. Open legal questions (need human/legal review before a public release)

- Trademark search for the name "SMY-OS" and its logo.
- Patent exposure of compatibility work (PE loader, NT-like services, file
  formats) — implement from public specs only; review before release.
- Redistribution of firmware blobs (none planned; OVMF is a test tool only).
- Export-control notes for cryptography (Phase 25–26).
