/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

/* x86 port I/O and CPU control primitives. */

#ifndef SMY_KERNEL_ARCH_X86_64_IO_H
#define SMY_KERNEL_ARCH_X86_64_IO_H

#include "core/compiler.h"

#include <stdint.h>

static inline void io_out8(uint16_t port, uint8_t value)
{
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port) : "memory");
}

static inline uint8_t io_in8(uint16_t port)
{
    uint8_t value;

    __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port) : "memory");
    return value;
}

static inline void cpu_pause(void)
{
    __asm__ volatile("pause");
}

/* Disable interrupts and halt this CPU permanently. */
static inline SMY_NORETURN void cpu_halt_forever(void)
{
    for (;;) {
        __asm__ volatile("cli; hlt");
    }
}

#endif /* SMY_KERNEL_ARCH_X86_64_IO_H */
