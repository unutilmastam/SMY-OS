/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

/*
 * x86-64 4-level page tables (PML4 -> PDPT -> PD -> PT): the table logic
 * behind arch/mmu.h (docs/kernel/memory.md, ADR 0012). Pure logic,
 * host-testable: page-table pages are obtained, reached and released only
 * through `struct paging_ops`, so tests run it on ordinary heap memory.
 *
 * Pages are 4 KiB; 2 MiB pages are supported for large linear mappings such
 * as the direct map. Intermediate tables are created on demand and are not
 * freed when they become empty (the kernel address space only grows; user
 * address spaces, Phase 10, release whole trees).
 *
 * Not thread-safe: callers serialise access (single CPU until Phase 16).
 */

#ifndef SMY_KERNEL_ARCH_X86_64_PAGING_H
#define SMY_KERNEL_ARCH_X86_64_PAGING_H

#include "arch/mmu.h"

#include <stdbool.h>
#include <stdint.h>

#define PAGING_PAGE_SIZE 4096u
#define PAGING_LARGE_PAGE_SIZE 0x200000u /* 2 MiB */
#define PAGING_ENTRIES 512u

/* Hardware page-table entry bits (Intel SDM Vol. 3A, 4.5). */
#define PTE_PRESENT (1ull << 0)
#define PTE_WRITABLE (1ull << 1)
#define PTE_USER (1ull << 2)
#define PTE_PWT (1ull << 3) /* with our PAT setup: write-combining (mmu.c) */
#define PTE_PCD (1ull << 4)
#define PTE_ACCESSED (1ull << 5)
#define PTE_DIRTY (1ull << 6)
#define PTE_LARGE (1ull << 7) /* PS: 2 MiB page in a PD entry */
#define PTE_GLOBAL (1ull << 8)
#define PTE_NO_EXECUTE (1ull << 63)
#define PTE_ADDRESS_MASK 0x000FFFFFFFFFF000ull

struct paging_ops {
    /* Allocate one 4 KiB physical page for a table; contents are zeroed by
     * the caller (paging.c). Returns false when out of memory. */
    bool (*alloc_table)(void *ctx, uint64_t *phys_out);
    /* Pointer through which the page at `phys` can be read and written. */
    void *(*table_ptr)(void *ctx, uint64_t phys);
    /* Drop any cached translation of `virt` (TLB) after a change; may be
     * NULL for an address space that is not active. */
    void (*invalidate)(void *ctx, uint64_t virt);
    void *ctx;
};

struct address_space {
    uint64_t root; /* physical address of the PML4 */
    const struct paging_ops *ops;
    uint64_t table_pages; /* tables allocated, root included */
};

/* The four table entries a walk visits; `levels` is how many were read
 * (the walk stops at a non-present or 2 MiB entry). For diagnostics. */
struct vm_walk {
    uint64_t entries[4]; /* PML4E, PDPTE, PDE, PTE */
    unsigned levels;
};

/* Allocate an empty root table. */
enum vm_status paging_init(struct address_space *as, const struct paging_ops *ops);

/* Map one 4 KiB page / one 2 MiB page. Both addresses must be aligned to the
 * page size and `virt` canonical. */
enum vm_status paging_map(struct address_space *as, uint64_t virt, uint64_t phys, unsigned flags);
enum vm_status paging_map_large(struct address_space *as, uint64_t virt, uint64_t phys,
                                unsigned flags);

/*
 * Map [virt, virt+size) to [phys, phys+size), using 2 MiB pages wherever
 * both addresses are 2 MiB aligned and a whole large page fits, 4 KiB pages
 * elsewhere. Stops at the first error (earlier pages stay mapped).
 */
enum vm_status paging_map_range(struct address_space *as, uint64_t virt, uint64_t phys,
                                uint64_t size, unsigned flags);

/* Remove the 4 KiB mapping of `virt`; returns the physical page it had. */
enum vm_status paging_unmap(struct address_space *as, uint64_t virt, uint64_t *phys_out);

/* Change the permissions of the existing 4 KiB mapping of `virt`. */
enum vm_status paging_protect(struct address_space *as, uint64_t virt, unsigned flags);

/* Translate `virt` (any alignment). VM_ERR_NOT_MAPPED if nothing is there. */
enum vm_status paging_translate(const struct address_space *as, uint64_t virt,
                                struct vm_translation *out);

/* Record the entries a hardware walk of `virt` would read. */
void paging_walk(const struct address_space *as, uint64_t virt, struct vm_walk *out);

/* True if `virt` is canonical for 4-level paging (bits 63..47 equal). */
bool paging_is_canonical(uint64_t virt);

#endif /* SMY_KERNEL_ARCH_X86_64_PAGING_H */
