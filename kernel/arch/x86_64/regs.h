/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

/* x86-64 control registers, MSRs, CPUID and descriptor-table registers. */

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

static inline void write_cr0(uint64_t v)
{
    __asm__ volatile("mov %0, %%cr0" : : "r"(v) : "memory");
}

static inline void write_cr3(uint64_t v)
{
    __asm__ volatile("mov %0, %%cr3" : : "r"(v) : "memory");
}

static inline void write_cr4(uint64_t v)
{
    __asm__ volatile("mov %0, %%cr4" : : "r"(v) : "memory");
}

enum {
    CR0_WP = 1u << 16,
    CR4_PGE = 1u << 7,
    CR4_SMEP = 1u << 20,
    CR4_SMAP = 1u << 21,
};

#define MSR_IA32_PAT 0x277u
#define MSR_IA32_EFER 0xC0000080u
#define EFER_NXE (1u << 11)

static inline uint64_t read_msr(uint32_t msr)
{
    uint32_t lo;
    uint32_t hi;

    __asm__ volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
    return ((uint64_t)hi << 32) | lo;
}

static inline void write_msr(uint32_t msr, uint64_t v)
{
    __asm__ volatile("wrmsr" : : "c"(msr), "a"((uint32_t)v), "d"((uint32_t)(v >> 32)) : "memory");
}

struct cpuid_regs {
    uint32_t eax, ebx, ecx, edx;
};

static inline struct cpuid_regs cpuid(uint32_t leaf, uint32_t subleaf)
{
    struct cpuid_regs r;

    __asm__ volatile("cpuid"
                     : "=a"(r.eax), "=b"(r.ebx), "=c"(r.ecx), "=d"(r.edx)
                     : "a"(leaf), "c"(subleaf));
    return r;
}

static inline void invlpg(uint64_t virt)
{
    __asm__ volatile("invlpg (%0)" : : "r"(virt) : "memory");
}

static inline void wbinvd(void)
{
    __asm__ volatile("wbinvd" : : : "memory");
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
