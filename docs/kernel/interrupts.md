# Interrupts, timer and clock

Status: **Implemented, Unit-tested, Emulator-tested** (Phase 7; QEMU q35,
SeaBIOS and OVMF, with and without HPET). Design:
[ADR 0014](../decisions/0014-interrupts-timer-keyboard.md).

Code:
- `kernel/acpi/acpi.{h,c}`: table parser;
- `kernel/arch/x86_64/apic.{h,c}`: 8259 PIC masking, local APIC, I/O APIC,
  HPET, PIT;
- `kernel/arch/x86_64/platform.c`: bring-up and dispatch;
- `kernel/arch/platform.h`: the architecture-neutral interface;
- `kernel/core/time.{h,c}` and `clock.c`: the clock.

## Bring-up (in `kmain_late`, after virtual memory)

1. ACPI: RSDP (physical address from Limine, base revision ≥ 3), then
   RSDT/XSDT, MADT and HPET. Checksums and lengths are verified. Tables
   in firmware-reserved memory are mapped read-only on demand
   (`vmm_map_physical`).
2. Both 8259 PICs are remapped to vectors 0xE0–0xEF and masked.
3. Local APIC: enabled, task priority 0, spurious vector 0xFF, error
   vector 0xFE. It uses MMIO mapped uncached, or MSRs in x2APIC mode.
4. I/O APIC(s): every pin is masked until a driver attaches to it.
5. HPET main counter enabled. The local APIC timer is calibrated over
   10 ms (HPET, or PIT channel 2 when there is none) and started in
   periodic mode at 1000 Hz on vector 0x20.
6. Keyboard driver attached to ISA IRQ 1, then `sti`. After "boot OK" the
   CPU idles with `sti; hlt`.

Log line (QEMU q35, SeaBIOS):

```
SMY-OS: platform: ACPI BOCHS  (rev 0), 1 CPU(s), 1 I/O APIC(s), local APIC xAPIC id 0; timer 1000 Hz calibrated with HPET (631313 counts/10 ms)
```

## Vectors

| Vector | Source |
|---|---|
| 0–31 | CPU exceptions (`docs/kernel/exceptions.md`) |
| 0x20 | local APIC timer |
| 0x30 + n | ISA IRQ n through the I/O APIC (keyboard: 0x31) |
| 0xE0–0xEF | masked 8259 PIC (never expected) |
| 0xFE | local APIC error |
| 0xFF | spurious (counted, no EOI) |

All 256 IDT gates exist and share `isr.S`. Exceptions go to
`exception_dispatch`, vectors ≥ 32 to `irq_dispatch`, which sends the EOI.
Handlers run with interrupts disabled. A panic disables interrupts first.

## API

```c
/* core/time.h */
uint64_t clock_monotonic_ns(void);   /* HPET-based; never decreases */
uint64_t clock_ticks(void);          /* timer interrupts since start */
void clock_sleep_ns(uint64_t ns);    /* hlt until the time has passed */

/* arch/platform.h */
bool arch_irq_attach_isa(unsigned irq, arch_irq_handler handler, void *ctx);
void arch_interrupts_enable(void), arch_interrupts_disable(void);
void arch_wait_for_interrupt(void);
```

## Tests

- **Host:**
  - `test_acpi.c` (5 tests: RSDT and XSDT paths, LAPIC address override,
    interrupt source overrides, bad checksums and lengths, malformed MADT
    entries, wrong HPET address space);
  - `test_time.c` (3 tests: exact conversion, no overflow at 2^64,
    rounding).
- **Kernel (`make ktest`):**
  - PICs masked, APIC enabled, all gates present;
  - clock monotonic over 20 000 reads;
  - timer rate against the clock (HPET) and against the PIT, each over
    200 ms (QEMU: 199 ticks, ±20 % allowed);
  - `make ktest-no-hpet` (QEMU `hpet=off`): the PIT path.

## Limits

- One CPU receives interrupts (SMP: Phase 16).
- Without an HPET the clock has 1 ms resolution.
- No MSI/MSI-X yet (PCI drivers, Phase 14+).
- The timer is periodic (no tickless mode).
