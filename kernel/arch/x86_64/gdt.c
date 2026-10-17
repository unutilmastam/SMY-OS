/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

#include "arch/x86_64/gdt.h"

#include "arch/x86_64/descriptors.h"

#include <stddef.h>

/* Implemented in cpu_asm.S. */
void gdt_load(const struct descriptor_pointer *gdtr);
void tss_load(uint16_t selector);

#define IST_STACK_SIZE 16384
#define IST_STACK_COUNT 3

/* Which IST slot each of ist_stacks[] serves. */
static const uint8_t ist_slots[IST_STACK_COUNT] = {IST_DOUBLE_FAULT, IST_NMI, IST_MACHINE_CHECK};

/* Separate stacks for exceptions that must work even when the current stack
 * is unusable (double fault after a stack overflow, NMI, machine check).
 * Guard pages below them come with the kernel's own page tables (Phase 5). */
SMY_ALIGNED(16) static uint8_t ist_stacks[IST_STACK_COUNT][IST_STACK_SIZE];

SMY_ALIGNED(16) static uint64_t gdt[GDT_ENTRIES];
SMY_ALIGNED(16) static struct tss64 tss;

/* GDT slot of a selector (selector = index * 8 | TI | RPL). */
static unsigned gdt_slot(unsigned selector)
{
    return selector >> 3;
}

void gdt_init(void)
{
    struct descriptor_pointer gdtr;

    gdt[gdt_slot(GDT_NULL)] = 0;
    gdt[gdt_slot(GDT_KERNEL_CODE)] =
        gdt_segment(0, 0xFFFFF, GDT_ACCESS_KERNEL_CODE, GDT_FLAGS_CODE64);
    gdt[gdt_slot(GDT_KERNEL_DATA)] =
        gdt_segment(0, 0xFFFFF, GDT_ACCESS_KERNEL_DATA, GDT_FLAGS_DATA);
    gdt[gdt_slot(GDT_USER_DATA)] = gdt_segment(0, 0xFFFFF, GDT_ACCESS_USER_DATA, GDT_FLAGS_DATA);
    gdt[gdt_slot(GDT_USER_CODE)] = gdt_segment(0, 0xFFFFF, GDT_ACCESS_USER_CODE, GDT_FLAGS_CODE64);

    tss = (struct tss64){0};
    for (size_t i = 0; i < IST_STACK_COUNT; i++) {
        /* tss.ist[] is 0-based; IST slot numbers are 1-based. */
        tss.ist[ist_slots[i] - 1] = (uint64_t)(uintptr_t)(ist_stacks[i] + IST_STACK_SIZE);
    }
    tss.iopb_offset = sizeof tss; /* no I/O permission bitmap */
    gdt_tss_descriptor(&gdt[gdt_slot(GDT_TSS)], (uint64_t)(uintptr_t)&tss, sizeof tss - 1);

    gdtr.limit = sizeof gdt - 1;
    gdtr.base = (uint64_t)(uintptr_t)gdt;
    gdt_load(&gdtr);
    tss_load(GDT_TSS);
}

bool gdt_ist_stack_bounds(uintptr_t addr, uintptr_t *lo, uintptr_t *hi)
{
    for (size_t i = 0; i < IST_STACK_COUNT; i++) {
        const uintptr_t base = (uintptr_t)ist_stacks[i];

        if (addr >= base && addr < base + IST_STACK_SIZE) {
            *lo = base;
            *hi = base + IST_STACK_SIZE;
            return true;
        }
    }
    return false;
}
