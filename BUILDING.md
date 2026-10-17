# Building SMY-OS

**Current state (Phase 2):** `make` links the SMY Kernel ELF, `make iso`
builds a bootable hybrid ISO, `make boot-test` boots it in QEMU four ways, and
`make check` runs everything CI runs.

## 1. Where builds run

- **GitHub Actions** (`ubuntu-24.04`) — the source of truth.
- **Claude Code cloud sessions** (Ubuntu 24.04 container) — development.

See `docs/architecture/environment.md` for measured tool versions.

## 2. Required tools

Install on Ubuntu 24.04:

```sh
sudo apt-get update
sudo apt-get install -y clang lld llvm clang-format clang-tidy cppcheck \
    libclang-rt-18-dev make python3 git curl xz-utils \
    qemu-system-x86 ovmf xorriso mtools nasm gdb
```

| Tool | Why |
|---|---|
| clang, lld, llvm | compile/link kernel and host tests (ADR 0003) |
| libclang-rt-18-dev | ASan/UBSan runtime for host unit tests |
| curl, xz-utils | download and unpack pinned Limine source (`tools/fetch-limine.sh`) |
| nasm | building Limine from source (ADR 0007) |
| clang-format, clang-tidy, cppcheck | format and static analysis |
| make | build system (ADR 0004) |
| python3 | host scripts (link check, later design-token generator) |
| qemu-system-x86, ovmf | run and test the OS (BIOS and UEFI) |
| xorriso, mtools | build the hybrid ISO and FAT images *(Phase 2)* |
| gdb | debugging through QEMU's gdbstub |

`tools/check-env.sh` verifies them and prints versions: it fails if a
Phase 1 tool is missing and warns about later-phase tools (`--all` makes
those fatal too).

## 3. Commands

```sh
tools/check-env.sh        # verify tools
tools/fetch-limine.sh     # download pinned Limine source (SHA-256 verified)
make                      # build the kernel -> build/kernel/smy-kernel.elf
make test                 # host unit tests (ASan/UBSan)
make format               # reformat C sources in place
make format-check         # fail if any C source is not clang-formatted
make lint                 # clang-tidy + cppcheck, every finding is an error
make check                # all of the above, as CI runs it
make clean                # remove build/
make V=1 ...              # show full command lines
make iso                  # build/smy-os.iso + build/SHA256SUMS (hybrid, reproducible)
make boot-test            # boot the ISO in QEMU: BIOS/UEFI x CD/raw disk
make run                  # boot the ISO in QEMU, serial on the terminal
tools/check-reproducible.sh   # build the ISO twice, require identical SHA-256
```

Build layout: `Makefile` includes `mk/toolchain.mk` (tools and flags),
`kernel/kernel.mk`, `boot/boot.mk` (Limine build, ISO),
`tests/unit/unit.mk` and `tests/integration/integration.mk`. Kernel objects
go to `build/kernel/`, Limine to `build/limine/`, host test objects to
`build/host/`. ISO layout: `docs/install/iso-spec.md`.

Outputs go to `build/` (git-ignored).

## 4. Documentation check (available now)

```sh
python3 tools/check-links.py
```

Checks that every relative link in every Markdown file points to an existing
file (and, for `#anchors`, an existing heading).

## 5. Debugging with GDB

```sh
make run QEMU_EXTRA="-s -S"     # QEMU waits for GDB on tcp::1234
gdb build/kernel/smy-kernel.elf \
    -ex 'target remote :1234' -ex 'break kmain' -ex 'continue'
```

## 6. Reproducibility

`SOURCE_DATE_EPOCH` is set from the last commit time; ISO file timestamps are
fixed; absolute paths are mapped out of debug info; tool versions are pinned
and logged. `tools/check-reproducible.sh` (also in CI) builds the ISO twice in
different directories and compares SHA-256. Details:
`docs/install/iso-spec.md` §5.
