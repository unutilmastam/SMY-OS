# SMY-OS Threat Model (v0)

Version 0, Phase 0. A planning document: **no mitigation listed here is
implemented yet**. Each row names the phase that implements and tests it.
Updated in every security-relevant phase (at the latest Phase 25).

## 1. Assets

| Asset | Why it matters |
|---|---|
| Kernel integrity (code, page tables, credentials) | Kernel compromise = total compromise |
| User data on disk | Confidentiality and integrity of user files |
| Other disks / operating systems on the machine | The installer must never damage them |
| User credentials (password hashes) | Account takeover |
| Package and release signing keys | Supply-chain compromise of all users |
| Build pipeline (CI, scripts, downloaded tools) | Malicious code inserted into releases |

## 2. Trust boundaries

1. **User space ↔ kernel** (syscalls, page faults, shared memory).
2. **Process ↔ process** (IPC channels, shared files).
3. **Kernel ↔ devices** (DMA, MMIO, device-provided data such as USB
   descriptors, network packets, disk contents).
4. **Untrusted files ↔ parsers** (ELF, PE, filesystems, images, fonts).
5. **Network ↔ system.**
6. **Firmware/bootloader ↔ kernel** (memory map, ACPI tables — trusted but
   validated for sanity).
7. **Third-party source/binaries ↔ build** (Limine, toolchain, apt packages).

## 3. Attackers considered

| Attacker | Capability | In scope from |
|---|---|---|
| Malicious or buggy local program | Runs arbitrary code in ring 3 | Phase 10 |
| Malicious file (executable, disk image, document) | Crafted input to parsers | Phase 9 |
| Network attacker | Sends arbitrary packets | Phase 22 |
| Malicious USB device | Crafted descriptors, HID input | Phase 28 |
| Compromised package / mirror | Serves modified packages | Phase 26 |
| Supply-chain (downloaded bootloader/tools) | Modified binaries | Phase 1 |
| Physical attacker with the powered-off machine | Boots other media, reads disk | documented only; disk encryption is future work |

Out of scope for now: side channels (Spectre-class) beyond basic mitigations,
malicious firmware, DMA attacks before an IOMMU driver exists.

## 4. Threats and planned mitigations

| # | Threat | Mitigation | Phase | Test |
|---|---|---|---|---|
| T1 | User program reads/writes kernel memory | Separate address spaces, U/S page bits, SMAP/SMEP, validated user pointers | 5, 10 | QEMU test: program touching kernel memory is killed, kernel survives |
| T2 | Kernel executes injected data | W^X, NX for data, read-only text | 5 | QEMU test: write to RO page faults |
| T3 | Kernel stack overflow / smashing | Guard pages, stack protector | 3, 5 | QEMU test overflowing a guarded stack |
| T4 | Malformed ELF/PE crashes or exploits the loader | Bounds-checked parsers, host fuzzing under ASan | 9 | 10 000-input mutation fuzz, no crash |
| T5 | Malformed filesystem/GPT image | Defensive parsers, host tests with corrupted images | 14, 29 | host tests |
| T6 | Kernel address leak defeats randomisation | No kernel pointers in user-visible output; KASLR considered later | 10+ | review + test |
| T7 | Privilege escalation via syscall bugs | Small syscall surface, argument validation, capability checks | 10, 25 | unit + QEMU tests |
| T8 | Installer destroys the wrong disk | Explicit plan, confirmation bound to disk identity, live mode read-only policy in storage layer, test runner refuses block devices | 30, 31 | QEMU: disk image hash unchanged |
| T9 | Tampered Limine/tool download | Pinned version + SHA-256 verification | 1 | script fails on hash mismatch |
| T10 | Tampered packages/updates | Ed25519 signatures with an established library | 26, 35 | signature verification tests |
| T11 | Weak password storage | Argon2id via an established library | 25, 33 | unit tests |
| T12 | App abuses ambient authority | Capability-based sandbox, manifests | 25 | QEMU tests |
| T13 | Network packet parsing bugs | Bounds-checked parsers, fuzzing | 22, 23 | host tests + fuzz |
| T14 | Secrets committed to git | Keys only in CI secrets; never in repository | always | review; secret scanning |

## 5. Failure modes (design rule)

- On an internal inconsistency the kernel **panics with a diagnostic**, it does
  not continue in an unknown state.
- A faulting user program is terminated; the kernel and other programs keep
  running.
- An installer step failure stops execution, logs it, and never writes the boot
  entry (`docs/INSTALLER_PROMPT.md` §10).
