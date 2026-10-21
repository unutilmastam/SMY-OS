/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

/* Local APIC, I/O APIC, legacy PIC, HPET and PIT (Intel SDM Vol. 3A ch. 11,
 * 82093AA I/O APIC datasheet, IA-PC HPET 1.0a, Intel 8254). */

#ifndef SMY_KERNEL_ARCH_X86_64_APIC_H
#define SMY_KERNEL_ARCH_X86_64_APIC_H

#include <stdbool.h>
#include <stdint.h>

/* Interrupt vectors used by the kernel. */
enum {
    VECTOR_TIMER = 0x20,
    VECTOR_ISA_BASE = 0x30, /* ISA IRQ n -> 0x30 + n */
    VECTOR_LAPIC_ERROR = 0xFE,
    VECTOR_SPURIOUS = 0xFF,
};

/* 8259A: remap away from the exception vectors and mask every line. */
void pic_disable(void);

/* Local APIC (xAPIC MMIO, or x2APIC MSRs if the firmware enabled x2APIC). */
void lapic_init(uint64_t phys);
uint32_t lapic_id(void);
bool lapic_is_x2apic(void);
void lapic_eoi(void);
/* Count of the timer in one-shot mode with divider 16 over the time a
 * caller-supplied wait takes. */
uint32_t lapic_timer_measure(void (*wait)(void));
void lapic_timer_start_periodic(uint32_t initial_count, uint8_t vector);

/* I/O APIC */
bool ioapic_init(uint64_t phys, uint32_t gsi_base);
bool ioapic_route(uint32_t gsi, uint8_t vector, uint16_t inti_flags, uint32_t dest_apic_id);
unsigned ioapic_count(void);

/* HPET main counter; false if absent or unusable (32-bit only). */
bool hpet_init(uint64_t phys);
uint64_t hpet_counter(void);
uint64_t hpet_period_fs(void);

/* PIT channel 2: busy-wait ~10 ms (calibration without an HPET). */
bool pit_wait_10ms(void);

#endif /* SMY_KERNEL_ARCH_X86_64_APIC_H */
