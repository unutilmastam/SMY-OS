# Testing SMY-OS

Rule: a feature's status (`STATUS.md`) is only as high as its tests prove.
CI (GitHub Actions) is the source of truth. A hang is a failure: every QEMU
test has a timeout.

## 1. Test levels

| Level | What | How | Introduced |
|---|---|---|---|
| Docs | Markdown relative links and anchors | `python3 tools/check-links.py` | Phase 0 |
| Host unit tests | Portable logic (string lib, allocators, parsers, data structures) | host clang, `-fsanitize=address,undefined`, in-repo minimal harness | Phase 1 |
| Static analysis | All C sources | `clang-tidy`, `cppcheck`, `-Werror` | Phase 1 |
| Format | All C sources | `clang-format --dry-run --Werror` | Phase 1 |
| Boot test | ISO boots, `SMY-OS: boot OK` on serial | QEMU headless, BIOS + UEFI (OVMF), CD + raw disk, 30 s timeout, `isa-debug-exit` | Phase 2 |
| In-kernel self-tests (`ktest`) | Kernel behaviour (exceptions, memory, scheduler…) | boot flag runs tests, results on serial, exit code via `isa-debug-exit` | Phase 3 |
| Fuzz tests | Untrusted-input parsers | mutation fuzzing on host under ASan | Phase 9 |
| Screenshot tests | Graphics, toolkit, desktop | QEMU `screendump` compared with reference images | Phase 18 |
| Install scenarios | Installer on disposable disk images | QEMU + OVMF, scripted | Phase 31 |
| Performance baselines | Scheduler, alloc, IPC, frame time, boot time | `tests/performance/`, compared only within the same environment | Phase 8+ |
| Hardware tests | Real PC | manual checklist, owner-approved | Phase 36 |

## 2. Test matrix

Status: — not applicable yet · ✗ not implemented · ✓ passing in CI.

| Area | Docs | Unit | Static | QEMU BIOS | QEMU UEFI | Fuzz | HW |
|---|---|---|---|---|---|---|---|
| Documentation | ✓ (local; CI after first push) | — | — | — | — | — | — |
| Build system | — | ✗ | ✗ | — | — | — | — |
| Boot / kernel entry | — | — | ✗ | ✗ | ✗ | — | ✗ |
| CPU / exceptions | — | — | ✗ | ✗ | ✗ | — | ✗ |
| Physical memory | — | ✗ | ✗ | ✗ | ✗ | — | ✗ |
| Virtual memory / heap | — | ✗ | ✗ | ✗ | ✗ | — | ✗ |
| Executable parsers (ELF/PE) | — | ✗ | ✗ | — | — | ✗ | — |

Rows are added as phases add components. Update this table in every phase.

## 3. Rules

- Never disable, delete or weaken a failing test. Find the root cause; "flaky"
  is not one.
- Tests use only disposable image files; any script that is given a block
  device path must refuse it.
- QEMU timeouts must allow for TCG (no KVM in cloud sessions).
- Report the real result of every check; if a check could not run in a
  session, say so and rely on CI.
