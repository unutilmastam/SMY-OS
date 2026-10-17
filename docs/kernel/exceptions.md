# CPU exceptions and panics

Status: **Implemented, Emulator-tested** (Phase 3, BIOS and UEFI).
Code: `kernel/arch/x86_64/isr.S`, `exceptions.c`, `kernel/core/panic.c`,
`kernel/core/stacktrace.c`.

## Path of an exception

1. The CPU looks up the IDT gate (vectors 0–31) and, for `#DF`, NMI and
   `#MC`, switches to the gate's IST stack (`docs/kernel/cpu.md`).
2. The stub in `isr.S` pushes a dummy error code where the CPU does not push
   one, then the vector number, then all 15 general-purpose registers, and
   calls `exception_dispatch(struct interrupt_frame *)`.
3. `exception_dispatch` either resumes at a ktest's expected recovery point
   (only while a ktest has armed `exceptions_expect()`), or reports a panic.
4. On return the stub restores the registers, drops vector and error code,
   and `iretq`s.

No exception is handled and continued in normal operation yet: page faults
become recoverable with virtual memory (Phase 5) and user-mode faults
terminate the process (Phase 10).

## Panic report format

Every line starts with `SMY-OS:` so the serial log stays greppable. Example
(real output of `make ktest-panic-ud`, Phase 3):

```
SMY-OS: PANIC: unhandled CPU exception #UD (invalid opcode), vector 6
SMY-OS:   RIP=ffffffff80004e40  CS=0008  RFLAGS=0000000000000086
SMY-OS:   RSP=ffffffff80023ff8  SS=0010  error code=0000000000000000
SMY-OS:   RAX=595e9fbd94fda766  RBX=ffffffff80006390  RCX=00000000000186a0
SMY-OS:   RDX=00000000000003f8  RSI=0000000000000000  RDI=000000000000000a
SMY-OS:   RBP=ffffffff80024000  R8 =000000000000004e  R9 =0000000100002601
SMY-OS:   R10=0000000100002600  R11=0000000000000000  R12=0000000000000005
SMY-OS:   R13=0000000000000000  R14=0000000000000000  R15=ffffffff80005642
SMY-OS:   CR0=0000000080010011  CR3=000000000ff7b000  CR4=0000000000000020
SMY-OS: stack trace:
SMY-OS:   #0  0xffffffff80004e40
SMY-OS:   #1  0xffffffff80003b3e
SMY-OS:   #2  0xffffffff80004295
SMY-OS:   #3  0xffffffff80004d3a
SMY-OS: (resolve addresses with: llvm-addr2line -f -e build/kernel/smy-kernel.elf <address>)
SMY-OS: system halted
```

(In ktest mode the last line is `SMY-OS: ktest: run aborted by panic` and
QEMU exits with status 35.) Resolved with `llvm-addr2line`:
`#0 ktest_trigger_ud`, `#1 ktest_run`, `#2 kmain`, `#3 _start`.

For a page fault an extra line decodes CR2 and the error code:

```
SMY-OS:   fault address (CR2) 0xffffffff90000000: page not present, read, kernel mode
```

A panic from C code (`panic("...")`) prints the message and the stack trace
of the caller, without registers:

```
SMY-OS: PANIC: ktest: deliberate panic with value 42
SMY-OS: stack trace:
SMY-OS:   #0  0xffffffff80003dc7
...
```

## Stack traces

Frame pointers are kept (`-fno-omit-frame-pointer`) and tail calls are not
turned into jumps (`-fno-optimize-sibling-calls`), so each C function has a
frame. The walker (`core/stacktrace.c`) follows `[fp] = previous fp,
[fp+8] = return address` only while the frame lies on a known kernel stack
(boot stack or an IST stack), is 8-byte aligned and moves up the stack, at
most 16 frames. Limits:

- The function that was running is shown as `#0` (its RIP); its caller only
  appears if the faulting function had set up a frame. Assembly leaf
  functions (like the ktest triggers) have none, so their C caller is
  skipped.
- Addresses are not symbolised in the kernel yet; use `llvm-addr2line`.

## Nested panics

A fault while printing a panic prints
`SMY-OS: PANIC: nested panic (fault while reporting a panic)` and stops
immediately, without touching the state that caused the first panic.

## Tests

`make ktest` (`docs/kernel/ktest.md`): `#UD`, `#BP`, `#PF` (with CR2 and
error code), `#GP`, `#DE` caught and resumed; 1000 consecutive exceptions;
GDT/TSS/IDT read back; and three panic runs whose report lines are matched
by `tests/integration/ktest.sh`. Not tested yet: `#DF`, NMI and `#MC` on
their IST stacks (no safe way to cause them before Phase 5).
