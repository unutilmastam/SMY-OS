/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

/*
 * Kernel virtual memory (docs/kernel/memory.md, ADR 0012): builds the
 * kernel's own address space at boot and hands out guarded kernel stacks.
 *
 * Kernel address-space layout (4-level paging, top half):
 *
 *   0xffff800000000000  direct map of physical memory (HHDM; same offset
 *                       as the bootloader's, so existing pointers stay valid)
 *   0xfffffe0000000000  scratch area for self-tests (ktest)
 *   0xffffff0000000000  kernel stacks, each below an unmapped guard page
 *   0xffffffff80000000  kernel image (text RX, rodata R, data/bss RW)
 */

#ifndef SMY_KERNEL_MEMORY_VMM_H
#define SMY_KERNEL_MEMORY_VMM_H

#include "arch/mmu.h"

#include <limine.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define VMM_SCRATCH_BASE 0xfffffe0000000000ull
#define VMM_SCRATCH_SIZE 0x0000004000000000ull /* 256 GiB */
#define VMM_STACK_BASE 0xffffff0000000000ull
#define VMM_STACK_AREA 0x0000004000000000ull /* 256 GiB */
#define VMM_MAX_STACKS 64

struct vmm_kernel_image {
    uint64_t phys_base; /* where the bootloader loaded the image */
    uint64_t virt_base; /* link address (kernel_start) */
};

/*
 * Build the kernel address space (direct map of every memory-map entry except
 * reserved and bad memory, with the framebuffer write-combining; kernel image
 * by section) and switch the CPU to it. Panics on failure. Logs one line.
 */
void vmm_init(const struct limine_memmap_response *memmap, uint64_t hhdm_offset,
              const struct vmm_kernel_image *image);

/* Protection features switched on by vmm_init(). */
struct arch_mmu_features vmm_features(void);

/*
 * Allocate a kernel stack of `pages` pages with an unmapped guard page below
 * it. Returns the top (initial stack pointer, 16-byte aligned); panics when
 * out of memory or stack slots.
 */
uintptr_t vmm_alloc_stack(size_t pages);

/* If `addr` lies on a stack from vmm_alloc_stack, return its bounds. */
bool vmm_stack_bounds(uintptr_t addr, uintptr_t *lo, uintptr_t *hi);

/* If `addr` lies in the guard page of such a stack, return that stack's
 * bounds (a hit means the stack overflowed). */
bool vmm_stack_guard_hit(uintptr_t addr, uintptr_t *lo, uintptr_t *hi);

/* Panic-report help for a fault at `addr`: stack-overflow hint and the
 * page-table walk. */
void vmm_describe_fault(uintptr_t addr);

#endif /* SMY_KERNEL_MEMORY_VMM_H */
