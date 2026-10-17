/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

#ifndef SMY_KERNEL_ARCH_X86_64_GDT_H
#define SMY_KERNEL_ARCH_X86_64_GDT_H

#include <stdbool.h>
#include <stdint.h>

/* Build and load the GDT and the TSS (with IST stacks) on this CPU. */
void gdt_init(void);

/* If `addr` lies on an IST stack, return its bounds. */
bool gdt_ist_stack_bounds(uintptr_t addr, uintptr_t *lo, uintptr_t *hi);

#endif /* SMY_KERNEL_ARCH_X86_64_GDT_H */
