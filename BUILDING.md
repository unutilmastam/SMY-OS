# Building SMY-OS

**Current state (Phase 1):** `make` compiles the kernel library objects
(no bootable kernel yet), `make test` runs the host unit tests, and
`make check` runs everything CI runs. Sections marked *(Phase N)* become real
in that phase.

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
| nasm | building Limine from source *(Phase 2, ADR 0007)* |
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
make                      # build kernel objects -> build/kernel/libsmy-kernel.a
make test                 # host unit tests (ASan/UBSan)
make format               # reformat C sources in place
make format-check         # fail if any C source is not clang-formatted
make lint                 # clang-tidy + cppcheck, every finding is an error
make check                # all of the above, as CI runs it
make clean                # remove build/
make V=1 ...              # show full command lines
make iso                  # (Phase 2) build/smy-os.iso, hybrid, reproducible
make run                  # (Phase 2) boot the ISO in QEMU, serial on stdout
```

Build layout: `Makefile` includes `mk/toolchain.mk` (tools and flags),
`kernel/kernel.mk` and `tests/unit/unit.mk`. Kernel objects go to
`build/kernel/`, host test objects to `build/host/`.

Outputs go to `build/` (git-ignored).

## 4. Documentation check (available now)

```sh
python3 tools/check-links.py
```

Checks that every relative link in every Markdown file points to an existing
file (and, for `#anchors`, an existing heading).

## 5. Debugging with GDB *(Phase 2)*

```sh
make run QEMU_EXTRA="-s -S"     # QEMU waits for GDB on tcp::1234
gdb build/kernel/smy-kernel.elf \
    -ex 'target remote :1234' -ex 'break kmain' -ex 'continue'
```

## 6. Reproducibility *(Phase 2)*

`SOURCE_DATE_EPOCH` is set from the last commit time; file order and
timestamps in the ISO are fixed; tool versions are pinned/logged. CI builds the
ISO twice and compares SHA-256.
