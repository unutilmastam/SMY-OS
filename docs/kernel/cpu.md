# CPU setup (x86-64)

Status: **Implemented, Unit-tested (encodings), Emulator-tested (ktest)** —
Phase 3. Code: `kernel/arch/x86_64/{gdt,idt,descriptors,cpu}.c`,
`cpu_asm.S`, `isr.S`.

## Boot stack

`entry.S` switches from the bootloader's stack to the kernel's own 64 KiB
boot stack (`kernel_boot_stack_bottom` … `kernel_boot_stack_top`, in `.bss`)
before calling `kmain`. Known bounds let the stack walker reject frame
pointers that are not on a kernel stack. There is **no guard page yet** — a
stack overflow silently corrupts `.bss` until Phase 5 maps guard pages.

## GDT

| Selector | Slot | Descriptor | Access | Flags |
|---|---|---|---|---|
| `0x00` | 0 | null | — | — |
| `0x08` | 1 | kernel code, 64-bit | `0x9A` | `0xA` (G, L) |
| `0x10` | 2 | kernel data | `0x92` | `0xC` (G, D/B) |
| `0x18` | 3 | user data (used as `0x1B`) | `0xF2` | `0xC` |
| `0x20` | 4 | user code, 64-bit (used as `0x23`) | `0xFA` | `0xA` |
| `0x28` | 5–6 | 64-bit TSS (16-byte system descriptor) | `0x89` | — |

User data precedes user code because `SYSRET` (Phase 10) loads
`SS = STAR[63:48] + 8` and `CS = STAR[63:48] + 16`. User segments exist now
but nothing runs in ring 3 yet.

`gdt_load` (in `cpu_asm.S`) loads GDTR, reloads DS/ES/SS with `0x10`, clears
FS/GS, and reloads CS with a far return (`retfq`). `ltr 0x28` loads the TSS.

## TSS and IST

The TSS has no I/O permission bitmap (`iopb_offset = 104`). `rsp0` is set in
Phase 10 when ring 3 exists. Interrupt Stack Table:

| IST | Used by | Stack |
|---|---|---|
| 1 | `#DF` double fault (vector 8) | 16 KiB |
| 2 | NMI (vector 2) | 16 KiB |
| 3 | `#MC` machine check (vector 18) | 16 KiB |

These exceptions must not run on a possibly broken stack. The IST stacks have
no guard pages yet (Phase 5).

## IDT

256 gates; vectors 0–31 point to the stubs in `isr.S` as 64-bit interrupt
gates (`0x8E`, DPL 0, selector `0x08`). Vectors 32–255 are not present until
interrupt controllers exist (Phase 7); interrupts stay disabled until then.

## Encodings

Pure encoders in `descriptors.c` are checked by host unit tests against
known values (e.g. kernel code `0x00AF9A000000FFFF`) and the in-kernel tests
read back GDTR, IDTR and the task register (`docs/kernel/ktest.md`).
