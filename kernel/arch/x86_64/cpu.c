/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

#include "arch/cpu.h"

#include "arch/x86_64/gdt.h"
#include "arch/x86_64/idt.h"
#include "arch/x86_64/io.h"
#include "memory/vmm.h"

/* Boot stack, defined in entry.S. */
extern uint8_t kernel_boot_stack_bottom[];
extern uint8_t kernel_boot_stack_top[];

/* QEMU's isa-debug-exit device (tests add -device isa-debug-exit,iobase=0xf4).
 * Writing v makes QEMU exit with status (v << 1) | 1. */
enum {
    QEMU_DEBUG_EXIT_PORT = 0xF4,
    QEMU_EXIT_SUCCESS = 0x10, /* exit status 33 */
    QEMU_EXIT_FAILURE = 0x11, /* exit status 35 */
};

void arch_cpu_init(void)
{
    gdt_init();
    idt_init();
}

void arch_cpu_init_late(void)
{
    gdt_use_guarded_ist_stacks();
}

void arch_halt_forever(void)
{
    cpu_halt_forever();
}

void arch_test_exit(bool success)
{
    io_out8(QEMU_DEBUG_EXIT_PORT, success ? QEMU_EXIT_SUCCESS : QEMU_EXIT_FAILURE);
    cpu_halt_forever();
}

bool arch_stack_bounds(uintptr_t addr, uintptr_t *lo, uintptr_t *hi)
{
    const uintptr_t boot_lo = (uintptr_t)kernel_boot_stack_bottom;
    const uintptr_t boot_hi = (uintptr_t)kernel_boot_stack_top;

    if (addr >= boot_lo && addr < boot_hi) {
        *lo = boot_lo;
        *hi = boot_hi;
        return true;
    }
    return vmm_stack_bounds(addr, lo, hi) || gdt_ist_stack_bounds(addr, lo, hi);
}
