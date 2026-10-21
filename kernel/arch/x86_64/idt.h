/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

#ifndef SMY_KERNEL_ARCH_X86_64_IDT_H
#define SMY_KERNEL_ARCH_X86_64_IDT_H

#include "arch/x86_64/descriptors.h"

/* Install interrupt gates for all 256 vectors (exceptions 0-31, interrupts
 * 32-255, dispatched by arch/x86_64/irq.c) and load the IDT. */
void idt_init(void);

/* The gate for `vector` as installed (for tests). */
const struct idt_gate *idt_gate(unsigned vector);

#endif /* SMY_KERNEL_ARCH_X86_64_IDT_H */
