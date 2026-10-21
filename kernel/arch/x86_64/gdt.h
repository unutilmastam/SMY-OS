/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

#ifndef SMY_KERNEL_ARCH_X86_64_GDT_H
#define SMY_KERNEL_ARCH_X86_64_GDT_H

#include <stdbool.h>
#include <stdint.h>

/* Build and load the GDT and the TSS (with IST stacks) on this CPU. */
void gdt_init(void);

/* Switch the TSS to guard-paged IST stacks (memory/vmm.h); call once the
 * kernel's own page tables are active. */
void gdt_use_guarded_ist_stacks(void);

/* Current stack top in IST slot `slot` (1-based), for tests. */
uint64_t gdt_ist_top(unsigned slot);

/* If `addr` lies on one of the early static IST stacks, return its bounds. */
bool gdt_ist_stack_bounds(uintptr_t addr, uintptr_t *lo, uintptr_t *hi);

#endif /* SMY_KERNEL_ARCH_X86_64_GDT_H */
