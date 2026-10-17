/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

#include "memory/pmm.h"

#define BITS_PER_WORD 64u

static uint64_t align_down(uint64_t v)
{
    return v & ~(uint64_t)(PMM_PAGE_SIZE - 1);
}

/* Round up; returns false on overflow. */
static bool align_up(uint64_t v, uint64_t *out)
{
    if (v > UINT64_MAX - (PMM_PAGE_SIZE - 1)) {
        return false;
    }
    *out = align_down(v + PMM_PAGE_SIZE - 1);
    return true;
}

/* End of [base, base+length), clamped instead of wrapping around. */
static uint64_t region_end(const struct pmm_region *r)
{
    return r->length > UINT64_MAX - r->base ? UINT64_MAX : r->base + r->length;
}

/* Software popcount: a compiler builtin could become a call into a runtime
 * library the kernel does not link (no POPCNT in the baseline x86-64 ISA). */
static unsigned popcount64(uint64_t x)
{
    x = x - ((x >> 1) & 0x5555555555555555u);
    x = (x & 0x3333333333333333u) + ((x >> 2) & 0x3333333333333333u);
    x = (x + (x >> 4)) & 0x0F0F0F0F0F0F0F0Fu;
    return (unsigned)((x * 0x0101010101010101u) >> 56);
}

static bool test_bit(const uint64_t *map, uint64_t frame)
{
    return (map[frame / BITS_PER_WORD] >> (frame % BITS_PER_WORD) & 1u) != 0;
}

static void set_bit(uint64_t *map, uint64_t frame)
{
    map[frame / BITS_PER_WORD] |= (uint64_t)1 << (frame % BITS_PER_WORD);
}

static void clear_bit(uint64_t *map, uint64_t frame)
{
    map[frame / BITS_PER_WORD] &= ~((uint64_t)1 << (frame % BITS_PER_WORD));
}

static bool frame_free(const struct pmm *pmm, uint64_t frame)
{
    return test_bit(pmm->managed, frame) && !test_bit(pmm->used, frame);
}

/* Mark frames [first, last) as not managed (reserved), clamped to coverage. */
static void reserve_frames(struct pmm *pmm, uint64_t first, uint64_t last)
{
    if (last > pmm->frame_count) {
        last = pmm->frame_count;
    }
    for (uint64_t f = first; f < last; f++) {
        clear_bit(pmm->managed, f);
        set_bit(pmm->used, f);
    }
}

uint64_t pmm_frames_to_cover(const struct pmm_region *regions, size_t count)
{
    uint64_t highest = 0;

    for (size_t i = 0; i < count; i++) {
        if (regions[i].type == PMM_REGION_USABLE) {
            const uint64_t end = align_down(region_end(&regions[i]));
            if (end > highest) {
                highest = end;
            }
        }
    }
    return highest >> PMM_PAGE_SHIFT;
}

size_t pmm_storage_bytes(uint64_t frame_count)
{
    const uint64_t words = (frame_count + BITS_PER_WORD - 1) / BITS_PER_WORD;

    return (size_t)(2 * words * sizeof(uint64_t));
}

bool pmm_find_storage(const struct pmm_region *regions, size_t count, size_t bytes,
                      uint64_t *phys_out)
{
    for (size_t i = 0; i < count; i++) {
        uint64_t start;
        uint64_t end;

        if (regions[i].type != PMM_REGION_USABLE ||
            !align_up(regions[i].base < PMM_PAGE_SIZE ? PMM_PAGE_SIZE : regions[i].base, &start)) {
            continue;
        }
        end = align_down(region_end(&regions[i]));
        if (end > start && end - start >= bytes) {
            *phys_out = start;
            return true;
        }
    }
    return false;
}

enum pmm_status pmm_init(struct pmm *pmm, const struct pmm_region *regions, size_t count,
                         void *storage, size_t storage_bytes, uint64_t storage_phys,
                         uint64_t storage_len)
{
    const uint64_t frames = pmm_frames_to_cover(regions, count);
    const size_t needed = pmm_storage_bytes(frames);

    if (pmm == NULL || (regions == NULL && count != 0) || storage == NULL ||
        storage_bytes < needed || (uintptr_t)storage % sizeof(uint64_t) != 0) {
        return PMM_ERR_BAD_ARGUMENT;
    }

    pmm->frame_count = frames;
    pmm->words = needed / 2 / sizeof(uint64_t);
    pmm->used = storage;
    pmm->managed = pmm->used + pmm->words;
    pmm->hint = 0;
    for (size_t w = 0; w < pmm->words; w++) {
        pmm->used[w] = UINT64_MAX;
        pmm->managed[w] = 0;
    }

    /* 1. Whole pages inside usable regions become free. */
    for (size_t i = 0; i < count; i++) {
        uint64_t first;

        if (regions[i].type != PMM_REGION_USABLE || !align_up(regions[i].base, &first)) {
            continue;
        }
        for (uint64_t f = first >> PMM_PAGE_SHIFT;
             f < (align_down(region_end(&regions[i])) >> PMM_PAGE_SHIFT) && f < frames; f++) {
            set_bit(pmm->managed, f);
            clear_bit(pmm->used, f);
        }
    }
    /* 2. Anything touched by a reserved region is reserved, even if a
     *    (buggy) map also lists it as usable. */
    for (size_t i = 0; i < count; i++) {
        uint64_t last;

        if (regions[i].type == PMM_REGION_USABLE) {
            continue;
        }
        if (!align_up(region_end(&regions[i]), &last)) {
            last = UINT64_MAX;
        }
        reserve_frames(pmm, regions[i].base >> PMM_PAGE_SHIFT, last >> PMM_PAGE_SHIFT);
    }
    /* 3. Frame 0 (a null physical address must never be valid) and the
     *    bitmaps' own frames. */
    reserve_frames(pmm, 0, 1);
    if (storage_len != 0) {
        uint64_t last;

        if (!align_up(storage_phys + storage_len, &last)) {
            return PMM_ERR_BAD_ARGUMENT;
        }
        reserve_frames(pmm, storage_phys >> PMM_PAGE_SHIFT, last >> PMM_PAGE_SHIFT);
    }

    pmm->managed_frames = 0;
    pmm->free_frames = 0;
    for (size_t w = 0; w < pmm->words; w++) {
        pmm->managed_frames += popcount64(pmm->managed[w]);
        pmm->free_frames += popcount64(pmm->managed[w] & ~pmm->used[w]);
    }
    return PMM_OK;
}

enum pmm_status pmm_alloc(struct pmm *pmm, uint64_t *phys_out)
{
    if (pmm == NULL || phys_out == NULL) {
        return PMM_ERR_BAD_ARGUMENT;
    }
    if (pmm->free_frames == 0) {
        return PMM_ERR_NO_MEMORY;
    }
    /* Next-fit: start where the last allocation succeeded, wrap once. */
    for (size_t n = 0; n < pmm->words; n++) {
        const size_t w = (pmm->hint + n) % pmm->words;
        const uint64_t free_bits = pmm->managed[w] & ~pmm->used[w];

        if (free_bits != 0) {
            const uint64_t frame =
                (uint64_t)w * BITS_PER_WORD + (uint64_t)__builtin_ctzll(free_bits);

            set_bit(pmm->used, frame);
            pmm->free_frames--;
            pmm->hint = w;
            *phys_out = frame << PMM_PAGE_SHIFT;
            return PMM_OK;
        }
    }
    return PMM_ERR_NO_MEMORY; /* unreachable if free_frames is right */
}

enum pmm_status pmm_alloc_contiguous(struct pmm *pmm, uint64_t count, uint64_t align_frames,
                                     uint64_t *phys_out)
{
    if (pmm == NULL || phys_out == NULL || count == 0 || align_frames == 0 ||
        (align_frames & (align_frames - 1)) != 0) {
        return PMM_ERR_BAD_ARGUMENT;
    }
    if (count > pmm->free_frames) {
        return PMM_ERR_NO_MEMORY;
    }
    for (uint64_t start = 0; start < pmm->frame_count && count <= pmm->frame_count - start;
         start += align_frames) {
        uint64_t run = 0;

        while (run < count && frame_free(pmm, start + run)) {
            run++;
        }
        if (run == count) {
            for (uint64_t f = start; f < start + count; f++) {
                set_bit(pmm->used, f);
            }
            pmm->free_frames -= count;
            *phys_out = start << PMM_PAGE_SHIFT;
            return PMM_OK;
        }
    }
    return PMM_ERR_NO_MEMORY;
}

/* Check that `phys` names a managed, currently allocated frame. */
static enum pmm_status check_freeable(const struct pmm *pmm, uint64_t phys)
{
    const uint64_t frame = phys >> PMM_PAGE_SHIFT;

    if (phys % PMM_PAGE_SIZE != 0 || frame >= pmm->frame_count || !test_bit(pmm->managed, frame)) {
        return PMM_ERR_INVALID_ADDRESS;
    }
    if (!test_bit(pmm->used, frame)) {
        return PMM_ERR_DOUBLE_FREE;
    }
    return PMM_OK;
}

enum pmm_status pmm_free(struct pmm *pmm, uint64_t phys)
{
    return pmm_free_contiguous(pmm, phys, 1);
}

enum pmm_status pmm_free_contiguous(struct pmm *pmm, uint64_t phys, uint64_t count)
{
    if (pmm == NULL || count == 0 || count > pmm->frame_count) {
        return PMM_ERR_BAD_ARGUMENT;
    }
    /* Validate everything first so a bad request changes nothing. */
    for (uint64_t i = 0; i < count; i++) {
        const enum pmm_status st = check_freeable(pmm, phys + i * PMM_PAGE_SIZE);

        if (st != PMM_OK) {
            return st;
        }
    }
    for (uint64_t i = 0; i < count; i++) {
        clear_bit(pmm->used, (phys >> PMM_PAGE_SHIFT) + i);
    }
    pmm->free_frames += count;
    return PMM_OK;
}

bool pmm_is_free(const struct pmm *pmm, uint64_t phys)
{
    const uint64_t frame = phys >> PMM_PAGE_SHIFT;

    return phys % PMM_PAGE_SIZE == 0 && frame < pmm->frame_count && frame_free(pmm, frame);
}

struct pmm_stats pmm_get_stats(const struct pmm *pmm)
{
    return (struct pmm_stats){
        .managed_frames = pmm->managed_frames,
        .free_frames = pmm->free_frames,
        .used_frames = pmm->managed_frames - pmm->free_frames,
        .frame_count = pmm->frame_count,
    };
}

const char *pmm_status_name(enum pmm_status status)
{
    switch (status) {
    case PMM_OK:
        return "ok";
    case PMM_ERR_NO_MEMORY:
        return "out of memory";
    case PMM_ERR_INVALID_ADDRESS:
        return "invalid address";
    case PMM_ERR_DOUBLE_FREE:
        return "double free";
    case PMM_ERR_BAD_ARGUMENT:
    default:
        return "bad argument";
    }
}
