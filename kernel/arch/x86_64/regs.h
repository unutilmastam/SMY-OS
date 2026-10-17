/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

/* Reading x86-64 control and descriptor registers. */

#ifndef SMY_KERNEL_ARCH_X86_64_REGS_H
#define SMY_KERNEL_ARCH_X86_64_REGS_H

#include "arch/x86_64/descriptors.h"

#include <stdint.h>

static inline uint64_t read_cr0(void)
{
    uint64_t v;

    __asm__ volatile("mov %%cr0, %0" : "=r"(v));
    return v;
}

static inline uint64_t read_cr2(void)
{
    uint64_t v;

    __asm__ volatile("mov %%cr2, %0" : "=r"(v));
    return v;
}

static inline uint64_t read_cr3(void)
{
    uint64_t v;

    __asm__ volatile("mov %%cr3, %0" : "=r"(v));
    return v;
}

static inline uint64_t read_cr4(void)
{
    uint64_t v;

    __asm__ volatile("mov %%cr4, %0" : "=r"(v));
    return v;
}

static inline uint16_t read_task_register(void)
{
    uint16_t v;

    __asm__ volatile("str %0" : "=r"(v));
    return v;
}

static inline struct descriptor_pointer read_idtr(void)
{
    struct descriptor_pointer p;

    __asm__ volatile("sidt %0" : "=m"(p));
    return p;
}

static inline struct descriptor_pointer read_gdtr(void)
{
    struct descriptor_pointer p;

    __asm__ volatile("sgdt %0" : "=m"(p));
    return p;
}

#endif /* SMY_KERNEL_ARCH_X86_64_REGS_H */
