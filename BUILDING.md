# Building SMY-OS

**Current state (Phase 0): there is nothing to build yet.** This file describes
the planned build so that Phase 1–2 implement one agreed procedure. Sections
marked *(Phase N)* become real in that phase.

## 1. Where builds run

- **GitHub Actions** (`ubuntu-24.04`) — the source of truth.
- **Claude Code cloud sessions** (Ubuntu 24.04 container) — development.

See `docs/architecture/environment.md` for measured tool versions.

## 2. Required tools

Install on Ubuntu 24.04:

```sh
sudo apt-get update
sudo apt-get install -y clang lld llvm clang-format clang-tidy cppcheck \
    make python3 git qemu-system-x86 ovmf xorriso mtools gdb
```

| Tool | Why |
|---|---|
| clang, lld, llvm | compile/link kernel and host tests (ADR 0003) |
| clang-format, clang-tidy, cppcheck | format and static analysis |
| make | build system (ADR 0004) |
| python3 | host scripts (link check, later design-token generator) |
| qemu-system-x86, ovmf | run and test the OS (BIOS and UEFI) |
| xorriso, mtools | build the hybrid ISO and FAT images *(Phase 2)* |
| gdb | debugging through QEMU's gdbstub |

`tools/check-env.sh` *(Phase 1)* verifies all of them and prints versions.

## 3. Commands *(Phase 1–2)*

```sh
tools/check-env.sh        # verify tools
tools/fetch-limine.sh     # download pinned Limine (SHA-256 verified)
make                      # build kernel
make test                 # host unit tests (ASan/UBSan)
make iso                  # build/smy-os.iso (hybrid, reproducible)
make run                  # boot the ISO in QEMU, serial on stdout
make check                # everything CI runs
make clean                # remove build/
```

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
