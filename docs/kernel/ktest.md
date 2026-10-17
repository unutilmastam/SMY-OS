# ktest — in-kernel self-tests

Status: **Implemented, Emulator-tested** (Phase 3). Code:
`kernel/core/ktest.{h,c}`, tests in `kernel/**/ktest_*.c`, runner
`tests/integration/ktest.sh`.

## Why

Host unit tests cover portable logic. Behaviour that needs the real CPU and
kernel environment (descriptor tables, exceptions, frame layout, later page
tables and the scheduler) is tested inside the kernel, in QEMU.

## Writing a test

```c
#include "core/ktest.h"

KTEST(cpu_catches_invalid_opcode)
{
    KTEST_ASSERT(condition);
    KTEST_ASSERT_EQ_U64(actual, expected);
}

KTEST_PANIC(panic_on_unhandled_page_fault)   /* must end in a panic */
{
    ...
}
```

Tests are registered at link time in the `.ktest` section (no list to
update). Failed assertions print `file:line: condition` and the test
continues; the test is reported as FAIL.

## Running

The kernel command line option `smy.ktest=<selection>` selects tests:

| Selection | Runs |
|---|---|
| `all` | every `KTEST` (not the `KTEST_PANIC` ones) |
| `<name>` | exactly that test, including a `KTEST_PANIC` |

Output:

```
SMY-OS: ktest: selection 'all'
SMY-OS: ktest: RUN  cpu_catches_invalid_opcode
SMY-OS: ktest: PASS cpu_catches_invalid_opcode
...
SMY-OS: ktest: 10 run, 10 passed, 0 failed
```

The run ends with `arch_test_exit()`: QEMU's `isa-debug-exit` device makes
QEMU exit with status **33** (all passed) or **35** (failure, no test
matched, or a panic). A hang is caught by the runner's timeout.

```sh
make ktest                # full suite on BIOS and UEFI + 3 panic checks
make ktest-bios           # one target at a time
tests/integration/ktest.sh uefi cpu_catches_page_fault_with_address pass
```

### How the option reaches the kernel

The normal ISO is used unchanged. `ktest.sh` passes a complete Limine
configuration through QEMU as an SMBIOS OEM string
(`-smbios type=11,value=limine:config:...`), which Limine reads before
looking at the disk (Limine CONFIG.md, "Config via SMBIOS"). That config
boots `fslabel(SMY-OS):/boot/smy-kernel.elf` with `cmdline: smy.ktest=...`.
Decision and trade-offs: [ADR 0009](../decisions/0009-ktest-and-smbios-config.md).

## Current tests (Phase 3)

| Test | Checks |
|---|---|
| `cpu_gdt_and_tss_are_loaded` | GDTR limit, task register = `0x28` |
| `cpu_idt_has_exception_gates_with_ist` | 32 gates, type, selector, handler; IST for `#DF`/NMI/`#MC` |
| `cpu_catches_invalid_opcode` | `#UD`, faulting RIP |
| `cpu_catches_breakpoint` | `#BP` (trap: RIP after `int3`) |
| `cpu_catches_page_fault_with_address` | `#PF`, CR2, error code (not present, read, kernel) |
| `cpu_catches_general_protection` | `#GP` on a non-canonical address, error code 0 |
| `cpu_catches_divide_error` | `#DE` |
| `cpu_survives_repeated_exceptions` | 1000 `#UD` in a row, registers and stack restored |
| `core_stack_trace_walks_kernel_frames` | walker finds ≥ 4 frames, all inside kernel text |
| `core_formatting_works_in_kernel` | formatter with kernel flags (va_list, 64-bit values) |
| `panic_on_unhandled_invalid_opcode` (panic) | full register dump + stack trace |
| `panic_on_unhandled_page_fault` (panic) | CR2 line |
| `panic_from_kernel_code` (panic) | `panic()` message and trace |
