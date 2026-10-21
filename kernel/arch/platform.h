/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

/*
 * Architecture-independent platform interface: interrupt controllers, the
 * periodic timer and the clock counter (docs/kernel/interrupts.md). x86-64:
 * arch/x86_64/platform.c (ACPI MADT, I/O APIC, local APIC, HPET or PIT).
 */

#ifndef SMY_KERNEL_ARCH_PLATFORM_H
#define SMY_KERNEL_ARCH_PLATFORM_H

#include <stdbool.h>
#include <stdint.h>

#define ARCH_TIMER_HZ 1000u

/*
 * Find the interrupt controllers and timers through ACPI (`rsdp_phys` from
 * the bootloader), take over interrupt delivery, calibrate and start the
 * periodic timer at ARCH_TIMER_HZ. Interrupts stay disabled; panics if the
 * platform lacks what the kernel needs. Logs a summary.
 */
void arch_platform_init(uint64_t rsdp_phys);

/* Free-running counter and its period (femtoseconds per count). */
uint64_t arch_clock_counter(void);
uint64_t arch_clock_period_fs(void);

/* Timer interrupts since arch_platform_init. */
uint64_t arch_timer_ticks(void);

/* Route legacy (ISA) interrupt line `irq` to `handler`, unmasked. Handlers
 * run with interrupts disabled; the controller is acknowledged after. */
typedef void (*arch_irq_handler)(void *ctx);
bool arch_irq_attach_isa(unsigned irq, arch_irq_handler handler, void *ctx);

/* Interrupts handled per ISA line since boot (diagnostics, tests). */
uint64_t arch_irq_count_isa(unsigned irq);

void arch_interrupts_enable(void);
void arch_interrupts_disable(void);

/* Sleep until the next interrupt (interrupts enabled). */
void arch_wait_for_interrupt(void);

#endif /* SMY_KERNEL_ARCH_PLATFORM_H */
