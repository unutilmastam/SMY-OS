/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

#include "arch/x86_64/descriptors.h"

_Static_assert(sizeof(struct idt_gate) == 16, "IDT gate must be 16 bytes");
_Static_assert(sizeof(struct tss64) == 104, "64-bit TSS must be 104 bytes");
_Static_assert(sizeof(struct descriptor_pointer) == 10, "GDTR/IDTR image must be 10 bytes");

uint64_t gdt_segment(uint32_t base, uint32_t limit, uint8_t access, uint8_t flags)
{
    return ((uint64_t)limit & 0xFFFFu) | (((uint64_t)base & 0xFFFFFFu) << 16) |
           ((uint64_t)access << 40) | ((((uint64_t)limit >> 16) & 0xFu) << 48) |
           (((uint64_t)flags & 0xFu) << 52) | ((((uint64_t)base >> 24) & 0xFFu) << 56);
}

void gdt_tss_descriptor(uint64_t out[2], uint64_t base, uint32_t limit)
{
    out[0] = gdt_segment((uint32_t)(base & 0xFFFFFFFFu), limit, GDT_ACCESS_TSS, 0);
    out[1] = base >> 32;
}

struct idt_gate idt_make_gate(uint64_t handler, uint16_t selector, uint8_t ist, uint8_t type_attr)
{
    return (struct idt_gate){
        .offset_low = (uint16_t)(handler & 0xFFFFu),
        .selector = selector,
        .ist = (uint8_t)(ist & 0x7u),
        .type_attr = type_attr,
        .offset_mid = (uint16_t)((handler >> 16) & 0xFFFFu),
        .offset_high = (uint32_t)(handler >> 32),
        .reserved = 0,
    };
}

uint64_t idt_gate_handler(const struct idt_gate *gate)
{
    return (uint64_t)gate->offset_low | ((uint64_t)gate->offset_mid << 16) |
           ((uint64_t)gate->offset_high << 32);
}
