# Development environment report

Recorded: 2026-10-17 (Phase 0). Re-check at the start of every phase; from
Phase 1 on, `tools/check-env.sh` automates this.

## 1. Where SMY-OS is built

The owner has no computer. There are exactly two build environments:

| Environment | Role | Persistence |
|---|---|---|
| Claude Code cloud session (Ubuntu container) | Development, local build and tests | Ephemeral — tools installed with `apt-get` are lost when the session ends |
| GitHub Actions `ubuntu-24.04` runner | **Source of truth** for build, tests, QEMU boot tests | Fresh VM per job |

Both are Ubuntu 24.04 LTS, so the same `apt` package versions are available in
both. Nothing is ever built or run on real hardware in this phase.

## 2. Cloud session host (measured)

| Item | Value |
|---|---|
| OS | Ubuntu 24.04.5 LTS |
| Kernel | Linux 6.18.44 (container VM) |
| CPU | Intel Xeon @ 2.10 GHz, x86_64, 4 vCPUs, 52-bit phys / 57-bit virt addresses |
| RAM | 15 GiB, no swap |
| Virtualisation for QEMU | **No `/dev/kvm`, no VMX flag** → QEMU runs with TCG (software emulation) only |

## 3. Tools (measured in the cloud session)

"Pre-installed" = present when the session started. "apt" = installed during
Phase 0 with `apt-get install qemu-system-x86 xorriso cppcheck mtools ovmf`.

| Tool | Version | Source | Needed for |
|---|---|---|---|
| clang | 18.1.3 (Ubuntu) | pre-installed | kernel and host C compiler (ADR 0003) |
| ld.lld | 18.1.3 (Ubuntu LLD) | pre-installed | kernel linker (ADR 0003) |
| clang-format | 18.1.3 | pre-installed | format check |
| clang-tidy | 18.1.3 | pre-installed | static analysis |
| gcc | 13.3.0 | pre-installed | not used for the kernel; available as a second host compiler for tests |
| nasm | 2.16.01 | pre-installed | not used (ADR 0002 chooses GNU-as/Intel-syntax via clang) |
| GNU make | 4.3 | pre-installed | build system (ADR 0004) |
| cmake | 3.28.3 | pre-installed | not used (ADR 0004) |
| ninja | 1.11.1 | pre-installed | not used |
| gdb | 15.1 | pre-installed | debugging via QEMU gdbstub |
| python3 | 3.13.16 | pre-installed | host scripts (link check, later token generator) |
| git | 2.43.0 | pre-installed | version control |
| qemu-system-x86_64 | 8.2.2 (Debian 1:8.2.2+ds-0ubuntu1.18) | apt | emulator, boot tests |
| xorriso | 1.5.6 | apt | ISO creation (Phase 2) |
| cppcheck | 2.13.0 | apt | static analysis |
| mtools | 4.0.43 | apt | FAT images (Phase 2+, ESP / test disks) |
| OVMF | 2024.02-2ubuntu0.10 (`/usr/share/OVMF/OVMF_CODE_4M.fd`, `OVMF_VARS_4M.fd`) | apt | UEFI firmware for QEMU |

Missing at session start: `qemu-system-x86_64`, `xorriso`, `cppcheck`,
`mtools`, OVMF. All were installed successfully with `apt-get`.

Phase 1 addition (measured 2026-10-17): host unit tests need the sanitizer
runtime **`libclang-rt-18-dev`** (not pre-installed; the ASan link failed
until it was installed with `apt-get`). `tools/check-env.sh` now checks for it.

Not yet needed and not checked: `musl-gcc` (Phase 15), MinGW-w64 (Phase 24).

## 4. GitHub Actions runner

The `ubuntu-24.04` runner image ships clang/LLVM, gcc, make, cmake, python3 and
git. QEMU, xorriso, cppcheck, mtools and OVMF are **not** guaranteed and are
installed by the workflow with `apt-get` (same Ubuntu 24.04 packages as above).
Whether `/dev/kvm` is usable on the runner is not verified yet (to be logged in
Phase 1). The boot tests must **not** depend on it — they run under TCG with
generous timeouts, and use KVM only if present.

Exact runner versions are logged by `tools/check-env.sh` in each CI run from
Phase 1 on. Until Phase 1 lands, the only CI job is the documentation link
check (`.github/workflows/docs.yml`).

## 5. Third-party components checked online

| Component | Version found | Date | Licence | Checked how |
|---|---|---|---|---|
| Limine bootloader | **v12.9.3** (latest tag) | 2026-10-06 | BSD-2-Clause (bundled parts: 0BSD, MIT, BSD-2/3, Apache-2.0 WITH LLVM-exception, CC0 — see its `3RDPARTY.md`) | `git ls-remote --tags` on the upstream repo + shallow clone of the tag, reading `COPYING`, `3RDPARTY.md`, `ChangeLog` |

Details and the decision: `docs/decisions/0005-bootloader-limine.md`.

## 6. Consequences

- All checks can run inside a cloud session after `apt-get` (no KVM → slower
  QEMU; boot test timeouts must allow for TCG).
- Ubuntu's clang 18 is not the newest LLVM release. It is the version both
  environments share without extra downloads; ADR 0003 records this and when to
  revisit.
