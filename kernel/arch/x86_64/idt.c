/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

#include "arch/x86_64/idt.h"

#include "arch/x86_64/exceptions.h"

/* Implemented in cpu_asm.S and isr.S. */
void idt_load(const struct descriptor_pointer *idtr);
extern const uint64_t isr_stub_table[IDT_ENTRIES];

SMY_ALIGNED(16) static struct idt_gate idt[IDT_ENTRIES];

static uint8_t ist_for_vector(unsigned vector)
{
    switch (vector) {
    case EXC_DOUBLE_FAULT:
        return IST_DOUBLE_FAULT;
    case EXC_NMI:
        return IST_NMI;
    case EXC_MACHINE_CHECK:
        return IST_MACHINE_CHECK;
    default:
        return IST_NONE;
    }
}

void idt_init(void)
{
    struct descriptor_pointer idtr;

    for (unsigned v = 0; v < IDT_ENTRIES; v++) {
        idt[v] = idt_make_gate(isr_stub_table[v], GDT_KERNEL_CODE, ist_for_vector(v),
                               IDT_GATE_INTERRUPT_DPL0);
    }
    idtr.limit = sizeof idt - 1;
    idtr.base = (uint64_t)(uintptr_t)idt;
    idt_load(&idtr);
}

const struct idt_gate *idt_gate(unsigned vector)
{
    return vector < IDT_ENTRIES ? &idt[vector] : &idt[0];
}
