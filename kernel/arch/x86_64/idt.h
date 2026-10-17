/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

#ifndef SMY_KERNEL_ARCH_X86_64_IDT_H
#define SMY_KERNEL_ARCH_X86_64_IDT_H

#include "arch/x86_64/descriptors.h"

/* Install gates for exception vectors 0-31 and load the IDT. Vectors 32-255
 * stay not-present until interrupt controllers exist (Phase 7). */
void idt_init(void);

/* The gate for `vector` as installed (for tests). */
const struct idt_gate *idt_gate(unsigned vector);

#endif /* SMY_KERNEL_ARCH_X86_64_IDT_H */
