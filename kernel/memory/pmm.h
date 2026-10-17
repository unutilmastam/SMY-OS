/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

/*
 * Physical memory manager core: a bitmap allocator for 4 KiB page frames
 * (ADR 0010, docs/kernel/memory.md). Pure logic, host-testable: it never
 * touches the memory it manages, only its own bitmaps.
 *
 * Two bitmaps cover frames 0 .. frame_count-1:
 *   managed  1 = the frame is usable RAM handed to the PMM
 *   used     1 = the frame is allocated (or not managed)
 * A frame is free exactly when managed = 1 and used = 0. Keeping "managed"
 * separately lets pmm_free() reject frames the PMM never owned (firmware
 * areas, the kernel image, the bitmaps themselves, frame 0).
 *
 * Not thread-safe: callers serialise access (single CPU until Phase 16).
 */

#ifndef SMY_KERNEL_MEMORY_PMM_H
#define SMY_KERNEL_MEMORY_PMM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PMM_PAGE_SIZE 4096u
#define PMM_PAGE_SHIFT 12u

enum pmm_region_type {
    PMM_REGION_USABLE,
    PMM_REGION_RESERVED,
};

/* One entry of the firmware/bootloader memory map. */
struct pmm_region {
    uint64_t base;
    uint64_t length;
    enum pmm_region_type type;
};

enum pmm_status {
    PMM_OK = 0,
    PMM_ERR_NO_MEMORY,
    PMM_ERR_INVALID_ADDRESS, /* misaligned, out of range or not managed */
    PMM_ERR_DOUBLE_FREE,
    PMM_ERR_BAD_ARGUMENT,
};

struct pmm {
    uint64_t *used;
    uint64_t *managed;
    uint64_t frame_count; /* frames covered by the bitmaps */
    size_t words;         /* 64-bit words per bitmap */
    uint64_t managed_frames;
    uint64_t free_frames;
    size_t hint; /* word index where the next search starts */
};

struct pmm_stats {
    uint64_t managed_frames; /* usable RAM under PMM control */
    uint64_t free_frames;
    uint64_t used_frames; /* managed and allocated */
    uint64_t frame_count; /* bitmap coverage */
};

/* Frames the bitmaps must cover: up to the end of the highest usable region. */
uint64_t pmm_frames_to_cover(const struct pmm_region *regions, size_t count);

/* Bytes of storage needed for both bitmaps (a multiple of 8). */
size_t pmm_storage_bytes(uint64_t frame_count);

/*
 * Pick a page-aligned place for `bytes` of bitmap storage inside a usable
 * region (the first one large enough, skipping frame 0). Returns false if
 * none fits.
 */
bool pmm_find_storage(const struct pmm_region *regions, size_t count, size_t bytes,
                      uint64_t *phys_out);

/*
 * Build the allocator. `storage` (8-byte aligned, >= pmm_storage_bytes)
 * holds the bitmaps; if it lies in managed RAM, pass its physical range in
 * storage_phys/storage_len so those frames are reserved (host tests pass 0).
 * Reserved regions win over usable ones where they overlap; partial pages of
 * usable regions are not used; frame 0 is never handed out.
 */
enum pmm_status pmm_init(struct pmm *pmm, const struct pmm_region *regions, size_t count,
                         void *storage, size_t storage_bytes, uint64_t storage_phys,
                         uint64_t storage_len);

enum pmm_status pmm_alloc(struct pmm *pmm, uint64_t *phys_out);

/* `count` physically contiguous free frames, the first aligned to
 * `align_frames` frames (a power of two, >= 1). */
enum pmm_status pmm_alloc_contiguous(struct pmm *pmm, uint64_t count, uint64_t align_frames,
                                     uint64_t *phys_out);

enum pmm_status pmm_free(struct pmm *pmm, uint64_t phys);

/* Frees all `count` frames or, if any of them is invalid, none. */
enum pmm_status pmm_free_contiguous(struct pmm *pmm, uint64_t phys, uint64_t count);

bool pmm_is_free(const struct pmm *pmm, uint64_t phys);
struct pmm_stats pmm_get_stats(const struct pmm *pmm);
const char *pmm_status_name(enum pmm_status status);

#endif /* SMY_KERNEL_MEMORY_PMM_H */
