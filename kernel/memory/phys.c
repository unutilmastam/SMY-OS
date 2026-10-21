/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

#include "memory/phys.h"

#include "core/log.h"
#include "core/panic.h"

#include <stddef.h>

/* Enough for real firmware maps (typically 10-100 entries). */
#define PHYS_MAX_REGIONS 512

static struct pmm_region regions[PHYS_MAX_REGIONS];
static size_t region_count;
static struct pmm kernel_pmm;
static uint64_t hhdm;

static const char *limine_type_name(uint64_t type)
{
    switch (type) {
    case LIMINE_MEMMAP_USABLE:
        return "usable";
    case LIMINE_MEMMAP_RESERVED:
        return "reserved";
    case LIMINE_MEMMAP_ACPI_RECLAIMABLE:
        return "ACPI reclaimable";
    case LIMINE_MEMMAP_ACPI_NVS:
        return "ACPI NVS";
    case LIMINE_MEMMAP_BAD_MEMORY:
        return "bad memory";
    case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE:
        return "bootloader reclaimable";
    case LIMINE_MEMMAP_EXECUTABLE_AND_MODULES:
        return "kernel and modules";
    case LIMINE_MEMMAP_FRAMEBUFFER:
        return "framebuffer";
    case LIMINE_MEMMAP_RESERVED_MAPPED:
        return "reserved (mapped)";
    default:
        return "unknown";
    }
}

void *phys_to_virt(uint64_t phys)
{
    return (void *)(uintptr_t)(phys + hhdm);
}

uint64_t phys_from_virt(const void *virt)
{
    return (uint64_t)(uintptr_t)virt - hhdm;
}

void phys_init(const struct limine_memmap_response *memmap, uint64_t hhdm_offset)
{
    uint64_t usable_bytes = 0;
    uint64_t storage_phys;
    size_t storage_bytes;
    enum pmm_status st;
    struct pmm_stats stats;

    if (memmap == NULL || memmap->entry_count == 0) {
        panic("no memory map from the bootloader");
        return; /* not reached; tells cppcheck that memmap is non-null below */
    }
    if (memmap->entry_count > PHYS_MAX_REGIONS) {
        panic("memory map has %llu entries, limit is %d", (unsigned long long)memmap->entry_count,
              PHYS_MAX_REGIONS);
    }
    hhdm = hhdm_offset;

    /*
     * Only USABLE entries are given to the allocator. Bootloader-reclaimable
     * memory still holds the Limine responses the kernel reads (framebuffer,
     * command line, this map) and is reclaimed once they are no longer
     * needed (after Phase 5). ACPI-reclaimable memory waits for the ACPI
     * code (Phase 7).
     */
    region_count = (size_t)memmap->entry_count;
    for (size_t i = 0; i < region_count; i++) {
        const struct limine_memmap_entry *e = memmap->entries[i];
        const unsigned long long start = e->base;
        const unsigned long long end = e->base + e->length;

        regions[i] = (struct pmm_region){
            .base = e->base,
            .length = e->length,
            .type = e->type == LIMINE_MEMMAP_USABLE ? PMM_REGION_USABLE : PMM_REGION_RESERVED,
        };
        if (e->type == LIMINE_MEMMAP_USABLE) {
            usable_bytes += e->length;
        }
        klog(LOG_DEBUG, "memmap: %016llx-%016llx %s", start, end, limine_type_name(e->type));
    }

    storage_bytes = pmm_storage_bytes(pmm_frames_to_cover(regions, region_count));
    if (!pmm_find_storage(regions, region_count, storage_bytes, &storage_phys)) {
        panic("no usable region can hold the %zu-byte page bitmap", storage_bytes);
    }
    st = pmm_init(&kernel_pmm, regions, region_count, phys_to_virt(storage_phys), storage_bytes,
                  storage_phys, storage_bytes);
    if (st != PMM_OK) {
        panic("physical memory manager init failed: %s", pmm_status_name(st));
    }

    stats = pmm_get_stats(&kernel_pmm);
    klog(LOG_INFO, "memory: %llu MiB usable in %zu map entries, %llu free pages, bitmap %zu KiB",
         (unsigned long long)(usable_bytes >> 20), region_count,
         (unsigned long long)stats.free_frames, (storage_bytes + 1023) / 1024);
}

enum pmm_status phys_alloc_page(uint64_t *phys_out)
{
    return pmm_alloc(&kernel_pmm, phys_out);
}

enum pmm_status phys_alloc_pages(uint64_t count, uint64_t align_pages, uint64_t *phys_out)
{
    return pmm_alloc_contiguous(&kernel_pmm, count, align_pages, phys_out);
}

enum pmm_status phys_free_page(uint64_t phys)
{
    return pmm_free(&kernel_pmm, phys);
}

enum pmm_status phys_free_pages(uint64_t phys, uint64_t count)
{
    return pmm_free_contiguous(&kernel_pmm, phys, count);
}

struct pmm_stats phys_stats(void)
{
    return pmm_get_stats(&kernel_pmm);
}

bool phys_page_in_usable_region(uint64_t phys)
{
    for (size_t i = 0; i < region_count; i++) {
        if (regions[i].type == PMM_REGION_USABLE && phys >= regions[i].base &&
            phys + PMM_PAGE_SIZE <= regions[i].base + regions[i].length) {
            return true;
        }
    }
    return false;
}
