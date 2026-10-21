/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

/*
 * The kernel's physical memory: boot-time setup and the global page-frame
 * allocator (docs/kernel/memory.md). Physical memory is reached through the
 * higher-half direct map (HHDM) that Limine sets up: virtual = physical +
 * offset. The kernel's own page tables (memory/vmm.c) keep the direct map at
 * the same offset.
 */

#ifndef SMY_KERNEL_MEMORY_PHYS_H
#define SMY_KERNEL_MEMORY_PHYS_H

#include "memory/pmm.h"

#include <limine.h>
#include <stdbool.h>
#include <stdint.h>

/*
 * Copy the Limine memory map, place the bitmaps, build the allocator and log
 * a summary. Panics if the map is missing or no storage fits.
 */
void phys_init(const struct limine_memmap_response *memmap, uint64_t hhdm_offset);

/* Direct-map access to a physical address, and back. */
void *phys_to_virt(uint64_t phys);
uint64_t phys_from_virt(const void *virt);

/* Global page-frame allocator (wrappers around memory/pmm.h). */
enum pmm_status phys_alloc_page(uint64_t *phys_out);
enum pmm_status phys_alloc_pages(uint64_t count, uint64_t align_pages, uint64_t *phys_out);
enum pmm_status phys_free_page(uint64_t phys);
enum pmm_status phys_free_pages(uint64_t phys, uint64_t count);
struct pmm_stats phys_stats(void);

/* True if [phys, phys+PMM_PAGE_SIZE) lies inside a USABLE map entry. */
bool phys_page_in_usable_region(uint64_t phys);

#endif /* SMY_KERNEL_MEMORY_PHYS_H */
