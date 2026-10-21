/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

/*
 * Architecture-independent MMU interface: the kernel address space
 * (docs/kernel/memory.md, ADR 0012). Generic code (memory/vmm.c) builds the
 * kernel's mappings through these calls; each architecture implements them
 * (x86-64: arch/x86_64/mmu.c on top of the 4-level page tables in
 * arch/x86_64/paging.c).
 */

#ifndef SMY_KERNEL_ARCH_MMU_H
#define SMY_KERNEL_ARCH_MMU_H

#include <stdbool.h>
#include <stdint.h>

#define ARCH_PAGE_SIZE 4096u

/* Mapping permissions and attributes. Every mapping is readable. */
enum {
    VM_READ = 0,
    VM_WRITE = 1u << 0,
    VM_EXEC = 1u << 1,
    VM_USER = 1u << 2,
    VM_WRITE_COMBINING = 1u << 3, /* framebuffers */
    VM_UNCACHED = 1u << 4,        /* MMIO */
};

enum vm_status {
    VM_OK = 0,
    VM_ERR_NO_MEMORY,      /* no page for a new table */
    VM_ERR_BAD_ARGUMENT,   /* misaligned, non-canonical or bad flags */
    VM_ERR_ALREADY_MAPPED, /* map over an existing mapping */
    VM_ERR_NOT_MAPPED,     /* unmap/protect/translate of a hole */
    VM_ERR_LARGE_PAGE,     /* a large page covers the small-page request */
};

/* What a virtual address translates to. */
struct vm_translation {
    uint64_t phys;      /* physical address of `virt` itself (offset included) */
    unsigned flags;     /* VM_* */
    uint64_t page_size; /* bytes covered by the mapping that holds `virt` */
};

/* CPU protection features found and switched on by arch_mmu_activate(). */
struct arch_mmu_features {
    bool no_execute;      /* x86: EFER.NXE */
    bool write_protect;   /* x86: CR0.WP (kernel writes honour read-only) */
    bool smep;            /* kernel may not execute user pages */
    bool smap;            /* kernel may not access user pages by accident */
    bool write_combining; /* x86: PAT entry for VM_WRITE_COMBINING */
};

const char *vm_status_name(enum vm_status status);

/* Create the empty kernel address space (not active yet). */
enum vm_status arch_mmu_init(void);

/* Map [virt, virt+size) -> [phys, phys+size) in the kernel address space,
 * with large pages where alignment allows. Page-aligned arguments only. */
enum vm_status arch_mmu_map(uint64_t virt, uint64_t phys, uint64_t size, unsigned flags);

/* Remove one page; returns the physical page it mapped. */
enum vm_status arch_mmu_unmap(uint64_t virt, uint64_t *phys_out);

/* Change the rights of one mapped page. */
enum vm_status arch_mmu_protect(uint64_t virt, unsigned flags);

enum vm_status arch_mmu_translate(uint64_t virt, struct vm_translation *out);

/*
 * Switch the CPU to the kernel address space and turn on the protection
 * features it supports. Panics if a required one (no-execute) is missing.
 */
struct arch_mmu_features arch_mmu_activate(void);

/* Print how the active tables translate `virt` (panic reports). */
void arch_mmu_describe(uint64_t virt);

/* Start `fn` on a new stack; the current stack is abandoned. */
_Noreturn void arch_switch_stack(uintptr_t stack_top, void (*fn)(void));

#endif /* SMY_KERNEL_ARCH_MMU_H */
