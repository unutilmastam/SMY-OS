# ADR 0014 — Interrupts and time: ACPI + APIC, HPET-calibrated LAPIC timer, HID key codes

- **Status:** Accepted
- **Date:** 2026-10-21
- **Phase:** 7

## Context

Phase 7 needs interrupt delivery, a periodic timer, a monotonic clock and
keyboard input, tested in QEMU (BIOS and UEFI) and later on real PCs. The
legacy 8259 PIC and PIT work everywhere but cannot route interrupts to
several CPUs (Phase 16) and are slow to program.

## Decision

1. **ACPI is required.** The RSDP comes from Limine. The parser
   (`kernel/acpi/acpi.c`) checks the checksum and length of every table.
   It is host-tested. It needs the MADT; HPET is optional.
2. **APIC only.** Both 8259s are remapped to 0xE0–0xEF and fully masked.
   Interrupts go through the I/O APIC(s) to the local APIC. ISA lines use
   the MADT's interrupt source overrides. The local APIC uses xAPIC MMIO,
   or x2APIC MSRs if the firmware has already switched to x2APIC.
3. **Vectors.** 0–31 are exceptions, 0x20 is the timer, 0x30 + n is ISA
   IRQ n, 0xFE is a local APIC error and 0xFF is spurious (no EOI). All 256
   IDT gates exist and share one entry path.
4. **Timer.** The local APIC timer runs in periodic mode at 1000 Hz with
   divider 16. It is calibrated over 10 ms against the HPET main counter,
   or against PIT channel 2 when there is no HPET.
5. **Monotonic clock.** The HPET main counter is converted to nanoseconds
   with overflow-free 64-bit arithmetic (`core/time.c`, host-tested). There
   is no 128-bit division, so no compiler runtime is needed. Without an
   HPET, the clock counts timer ticks (1 ms resolution).
6. **Keyboard.** The PS/2 controller is initialised with bounded waits, so
   a missing controller is reported instead of hanging. Translation gives
   scancode set 1. A host-tested decoder handles E0, E1 (Pause), the fake
   shifts around Print Screen and controller replies. It turns scancodes
   into **USB HID usage IDs**: the physical key, independent of any layout.
   The interrupt handler pushes events into a lock-free single-producer
   single-consumer ring of 64 events. When the ring is full, new events are
   dropped and counted.

## Alternatives considered

- **Keep the 8259 PIC:** simpler, but a dead end for SMP and for PCs without
  a usable legacy PIC; rejected.
- **TSC as the clock:** fastest. However, it needs invariant-TSC detection
  and calibration, and it behaves unreliably under some hypervisors. It
  can be added later as a faster clock source behind the same API.
- **x2APIC everywhere:** not all target CPUs support it. xAPIC with
  automatic x2APIC fallback covers both.
- **Linux keycodes:** they would tie the API to another kernel's
  numbering. HID usages are a public standard and match USB keyboards
  (a later phase) one to one.

## Consequences

- A PC without ACPI is not supported (every x86-64 PC from the last ~20
  years has ACPI).
- Only one CPU receives interrupts until SMP (Phase 16).
- With USB-only keyboards on real hardware, the firmware's PS/2 emulation
  is used until a USB stack exists (Phase 26-27).
