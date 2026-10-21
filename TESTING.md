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
| Boot test | ISO boots, `SMY-OS: boot OK` and the right firmware type on serial | `tests/integration/boot_test.sh`: QEMU q35 headless, BIOS (SeaBIOS) + UEFI (OVMF), CD + raw disk, 60 s timeout (TCG) | Phase 2 |
| Reproducible ISO | Two clean builds at different absolute paths give the same SHA-256 | `tools/check-reproducible.sh` | Phase 2 |
| In-kernel self-tests (`ktest`) | Kernel behaviour (exceptions, memory, scheduler…) | `make ktest`: `smy.ktest=` via an SMBIOS-supplied Limine config, results on serial, exit status 33/35 via `isa-debug-exit`, timeout = failure ([docs/kernel/ktest.md](docs/kernel/ktest.md)) | Phase 3 |
| Fuzz tests | Untrusted-input parsers | mutation fuzzing on host under ASan | Phase 9 |
| Screenshot tests | Graphics, toolkit, desktop | QEMU `screendump` compared with reference images | Phase 18 |
| Install scenarios | Installer on disposable disk images | QEMU + OVMF, scripted | Phase 31 |
| Performance baselines | Scheduler, alloc, IPC, frame time, boot time | `tests/performance/`, compared only within the same environment | Phase 8+ |
| Hardware tests | Real PC | manual checklist, owner-approved | Phase 36 |

## 2. Test matrix

Status: — not applicable yet · ✗ not implemented · ✓ passing in CI.

| Area | Docs | Unit | Static | QEMU BIOS | QEMU UEFI | Fuzz | HW |
|---|---|---|---|---|---|---|---|
| Documentation | ✓ | — | — | — | — | — | — |
| Build system (`make check`) | — | ✓ | ✓ | — | — | — | — |
| Kernel string library (`kernel/core/string.c`) | — | ✓ | ✓ | ✓ (linked, boots) | ✓ (linked, boots) | — | ✗ |
| Number formatting (`kernel/core/fmt.c`) | — | ✓ | ✓ | ✓ | ✓ | — | ✗ |
| Boot framebuffer (`kernel/drivers/bootfb`) | — | ✓ | ✓ | ✓ (`make boot-screen`: logo pixels via QEMU screendump) | ✓ (same) | — | ✗ |
| Boot / kernel entry, serial log, ISO (CD + raw disk) | — | — | ✓ | ✓ | ✓ | — | ✗ |
| Reproducible ISO | — | — | — | ✓ | ✓ | — | — |
| Design assets (`design/`: SVG rules, generated logos up to date, manifest, contrast, PNG exports) | — | ✓ (`make brand-check`, `make brand`) | — | — | — | — | — |
| CPU / exceptions (GDT, TSS, IDT, `#UD #BP #PF #GP #DE`, panic report) | — | ✓ (descriptor encodings) | ✓ | ✓ (ktest) | ✓ (ktest) | — | ✗ |
| Kernel log, formatter, command line, stack walker | — | ✓ | ✓ | ✓ (ktest) | ✓ (ktest) | — | ✗ |
| Physical memory (`memory/pmm`, `memory/phys`) | — | ✓ (incl. 20 000-step model test) | ✓ | ✓ (ktest, 256 MiB) | ✓ (ktest, 256 MiB and 4 GiB) | — | ✗ |
| Virtual memory (`arch/x86_64/paging`, `mmu`, `memory/vmm`) | — | ✓ (9 tests) | ✓ | ✓ (ktest, panic) | ✓ (ktest, `-cpu max`, 4 GiB) | — | ✗ |
| Design tokens and themes (`design/tokens/`) | — | ✓ (`make design-check`: palette, 147 contrast pairs, generator rules, header compile, font coverage) | — | — | — | — | — |
| Kernel heap (`memory/heap`, `kmalloc`) | — | ✓ (7 tests, random model) | ✓ | ✓ (ktest) | ✓ (ktest, double-free panic) | — | ✗ |
| ACPI tables (`kernel/acpi`) | — | ✓ (5 tests) | ✓ | ✓ (SeaBIOS) | ✓ (OVMF) | — | ✗ |
| Interrupts, timer, clock (`arch/x86_64/apic`, `platform`, `core/time`) | — | ✓ (time arithmetic) | ✓ | ✓ (ktest, `hpet=off`) | ✓ (ktest) | — | ✗ |
| Keyboard (`drivers/input`, `drivers/ps2`) | — | ✓ (6 tests) | ✓ | ✓ (QEMU `sendkey`) | ✓ (QEMU `sendkey`) | — | ✗ |
| Executable parsers (ELF/PE) | — | ✗ | ✗ | — | — | ✗ | — |

Rows are added as phases add components. Update this table in every phase.

## 3. Host unit tests (Phase 1)

- Harness: `tests/unit/harness/smytest.{h,c}` — in-repo, about 150 lines, no
  external dependency (reason: a framework would add a licence and a download
  for a few macros). `SMY_TEST(name) { ... }` registers a test;
  `SMY_EXPECT`, `SMY_EXPECT_EQ_U64`, `SMY_EXPECT_EQ_PTR` record failures and
  continue. The runner exits non-zero if any test fails or if no test ran.
  `build/host/tests/unit/smy-unit-tests <substring>` runs a subset.
- Kernel sources are compiled for the host with `-DSMY_HOST_TEST`, which
  renames their public symbols (`memcpy` → `smy_host_memcpy`, see
  `kernel/core/string.h`), and with `-fno-builtin`, so the test exercises our
  code and not the host C library.
- Sanitizers: `-fsanitize=address,undefined -fno-sanitize-recover=all`.
- New test file → add it to `UNIT_TEST_SOURCES` in `tests/unit/unit.mk`;
  new kernel source under test → `UNIT_KERNEL_SOURCES`.

Manual check (owner, 2026-10-18): the Phase 2 ISO (commit 91d858f) booted in
VirtualBox on the owner's Windows 11 laptop (BIOS mode) and showed the boot
screen. This is an additional emulator, not hardware testing.

Negative checks done once in Phase 7 (not committed): keyboard test with
`x` typed instead of `y` → FAIL "event 4: key 0x1b down, expected 0x1c
down"; no keys typed → FAIL after the 20 s timeout.

Negative checks done once in Phase 6 (not committed): light accent set back
to `#087F83` → contrast test FAIL (4.48:1 and 4.19:1); a raw `#FF0000` in a
theme → rejected ("not a base or brand colour"); IPAGothic → font coverage
FAIL (9 characters missing, incl. U+02BB and Uzbek Cyrillic).

Negative checks done once in Phase 5 (not committed): page tables without
the NX bit → 3 ktests fail (exact rights, W^X sweep, NX fetch); the guard
page below each stack mapped → `mmu_kernel_stacks_have_guard_pages` fails.

Negative checks done once in Phase 5B (not committed): a kernel that draws
no logo → `make boot-screen-bios` FAIL ("logo stroke missing", "logo core
missing"); a screen in the old interim background `#12151A` → FAIL on every
corner pixel. The RLE blitter rejects truncated runs, zero-length runs and
wrong pixel counts without drawing (host tests).

Negative checks done once in Phase 4 (not committed): removing the
double-free check → 2 host tests fail; not reserving frame 0 → 6 host tests
fail; losing one frame of accounting per free → the kernel leak test fails
(`free_frames == 0, expected 0xfe36`).

Negative checks done once in Phase 3 (not committed): a failing ktest
assertion → exit 35, runner FAIL; a `KTEST_PANIC` test that does not panic →
FAIL "expected a panic"; an endless loop in a test → FAIL "timeout"; an
unknown selection → "no test matches", FAIL; `isr.S` restoring RAX/RBX in the
wrong order → the suite panics with `#GP`, FAIL.

Negative checks done once in Phase 2 (not committed): a kernel that panics
before "boot OK" → `boot_test` FAIL "panic"; a kernel that halts before
"boot OK" → FAIL "timeout"; a device node (`/dev/null`) as image → refused
(exit 2). Before the prefix-map fix, `tools/check-reproducible.sh` failed on
real differences (Limine build-ids), so it detects non-reproducible builds.

Negative checks done once in Phase 1 (not committed): a wrong assertion, a
heap overflow (ASan), a format violation, an uninitialised return
(clang-tidy and cppcheck) and a wrong Limine SHA-256 each made the matching
command exit non-zero.

## 4. Rules

- Never disable, delete or weaken a failing test. Find the root cause; "flaky"
  is not one.
- Tests use only disposable image files; any script that is given a block
  device path must refuse it.
- QEMU timeouts must allow for TCG (no KVM in cloud sessions).
- Report the real result of every check; if a check could not run in a
  session, say so and rely on CI.
