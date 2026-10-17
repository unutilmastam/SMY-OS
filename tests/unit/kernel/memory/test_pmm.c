/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

/* Host tests for kernel/memory/pmm.c with fake memory maps. */

#include "harness/smytest.h"
#include "memory/pmm.h"

#include <stdlib.h>
#include <string.h>

#define PAGE ((uint64_t)PMM_PAGE_SIZE)
#define MIB ((uint64_t)1 << 20)

/* A small PC-like map: low memory, a hole, a reserved area inside RAM, an
 * unaligned usable region and a high region above a gap. */
static const struct pmm_region pc_map[] = {
    {.base = 0x0, .length = 0x9F000, .type = PMM_REGION_USABLE},         /* 159 frames */
    {.base = 0x9F000, .length = 0x61000, .type = PMM_REGION_RESERVED},   /* BIOS area */
    {.base = 0x100000, .length = 0x700000, .type = PMM_REGION_USABLE},   /* 1792 frames */
    {.base = 0x300000, .length = 0x2000, .type = PMM_REGION_RESERVED},   /* 2 frames inside */
    {.base = 0x800800, .length = 0x3000, .type = PMM_REGION_USABLE},     /* unaligned: 2 whole */
    {.base = 0x1000000, .length = 0x100000, .type = PMM_REGION_USABLE},  /* 256 frames */
    {.base = 0xFEC00000, .length = 0x1000, .type = PMM_REGION_RESERVED}, /* MMIO above top */
};
#define PC_MAP_COUNT (sizeof pc_map / sizeof pc_map[0])
/* usable whole frames: 159 + 1792 + 2 + 256 = 2209; minus reserved 2 inside,
 * minus frame 0 = 2206 */
#define PC_MAP_FREE 2206u

struct fixture {
    struct pmm pmm;
    void *storage;
};

static struct fixture make(const struct pmm_region *map, size_t n)
{
    struct fixture fx;
    const size_t bytes = pmm_storage_bytes(pmm_frames_to_cover(map, n));

    fx.storage = malloc(bytes > 0 ? bytes : 8);
    SMY_EXPECT(fx.storage != NULL);
    SMY_EXPECT_EQ_U64(pmm_init(&fx.pmm, map, n, fx.storage, bytes > 0 ? bytes : 8, 0, 0), PMM_OK);
    return fx;
}

static void destroy(struct fixture *fx)
{
    free(fx->storage);
}

static int in_usable_whole_page(uint64_t phys)
{
    for (size_t i = 0; i < PC_MAP_COUNT; i++) {
        const struct pmm_region *r = &pc_map[i];

        if (r->type == PMM_REGION_RESERVED && phys + PAGE > r->base && phys < r->base + r->length) {
            return 0;
        }
    }
    for (size_t i = 0; i < PC_MAP_COUNT; i++) {
        const struct pmm_region *r = &pc_map[i];

        if (r->type == PMM_REGION_USABLE && phys >= r->base && phys + PAGE <= r->base + r->length) {
            return 1;
        }
    }
    return 0;
}

SMY_TEST(pmm_init_counts_only_whole_usable_frames)
{
    struct fixture fx = make(pc_map, PC_MAP_COUNT);
    const struct pmm_stats st = pmm_get_stats(&fx.pmm);

    SMY_EXPECT_EQ_U64(st.frame_count, 0x1100000 / PAGE);
    SMY_EXPECT_EQ_U64(st.managed_frames, PC_MAP_FREE);
    SMY_EXPECT_EQ_U64(st.free_frames, PC_MAP_FREE);
    SMY_EXPECT_EQ_U64(st.used_frames, 0);
    SMY_EXPECT(!pmm_is_free(&fx.pmm, 0));        /* frame 0 */
    SMY_EXPECT(!pmm_is_free(&fx.pmm, 0x300000)); /* reserved inside RAM */
    SMY_EXPECT(!pmm_is_free(&fx.pmm, 0x800000)); /* partial page of unaligned region */
    SMY_EXPECT(pmm_is_free(&fx.pmm, 0x801000));
    SMY_EXPECT(pmm_is_free(&fx.pmm, 0x802000));
    SMY_EXPECT(!pmm_is_free(&fx.pmm, 0x803000)); /* partial tail */
    SMY_EXPECT(!pmm_is_free(&fx.pmm, 0xA0000));  /* BIOS area */
    destroy(&fx);
}

SMY_TEST(pmm_allocates_every_free_frame_once_then_reports_exhaustion)
{
    struct fixture fx = make(pc_map, PC_MAP_COUNT);
    unsigned char *seen = calloc((size_t)fx.pmm.frame_count, 1);
    uint64_t phys;
    uint64_t count = 0;

    SMY_EXPECT(seen != NULL);
    if (seen == NULL) {
        destroy(&fx);
        return;
    }
    while (pmm_alloc(&fx.pmm, &phys) == PMM_OK) {
        const uint64_t frame = phys / PAGE;

        SMY_EXPECT(phys % PAGE == 0);
        SMY_EXPECT(frame < fx.pmm.frame_count);
        SMY_EXPECT(in_usable_whole_page(phys));
        SMY_EXPECT(phys != 0);
        SMY_EXPECT(seen[frame] == 0);
        seen[frame] = 1;
        count++;
    }
    SMY_EXPECT_EQ_U64(count, PC_MAP_FREE);
    SMY_EXPECT_EQ_U64(pmm_alloc(&fx.pmm, &phys), PMM_ERR_NO_MEMORY);
    SMY_EXPECT_EQ_U64(pmm_alloc_contiguous(&fx.pmm, 1, 1, &phys), PMM_ERR_NO_MEMORY);
    free(seen);
    destroy(&fx);
}

SMY_TEST(pmm_free_validates_addresses)
{
    struct fixture fx = make(pc_map, PC_MAP_COUNT);
    uint64_t phys = 0;

    SMY_EXPECT_EQ_U64(pmm_alloc(&fx.pmm, &phys), PMM_OK);
    SMY_EXPECT_EQ_U64(pmm_free(&fx.pmm, phys + 8), PMM_ERR_INVALID_ADDRESS);
    SMY_EXPECT_EQ_U64(pmm_free(&fx.pmm, 0), PMM_ERR_INVALID_ADDRESS);
    SMY_EXPECT_EQ_U64(pmm_free(&fx.pmm, 0x300000), PMM_ERR_INVALID_ADDRESS);   /* reserved */
    SMY_EXPECT_EQ_U64(pmm_free(&fx.pmm, 0xFEC00000), PMM_ERR_INVALID_ADDRESS); /* beyond */
    SMY_EXPECT_EQ_U64(pmm_free(&fx.pmm, phys), PMM_OK);
    SMY_EXPECT_EQ_U64(pmm_free(&fx.pmm, phys), PMM_ERR_DOUBLE_FREE);
    SMY_EXPECT_EQ_U64(pmm_get_stats(&fx.pmm).free_frames, PC_MAP_FREE);
    SMY_EXPECT_EQ_U64(pmm_alloc(NULL, &phys), PMM_ERR_BAD_ARGUMENT);
    SMY_EXPECT_EQ_U64(pmm_alloc(&fx.pmm, NULL), PMM_ERR_BAD_ARGUMENT);
    destroy(&fx);
}

SMY_TEST(pmm_contiguous_respects_alignment_holes_and_reserved_frames)
{
    struct fixture fx = make(pc_map, PC_MAP_COUNT);
    uint64_t a = 0;
    uint64_t b = 0;

    /* 512 frames aligned to 512 (2 MiB): the 1 MiB..8 MiB region has a
     * reserved hole at 3 MiB, so the first fit is at 4 MiB. */
    SMY_EXPECT_EQ_U64(pmm_alloc_contiguous(&fx.pmm, 512, 512, &a), PMM_OK);
    SMY_EXPECT_EQ_U64(a, 4 * MIB);
    for (uint64_t i = 0; i < 512; i++) {
        SMY_EXPECT(!pmm_is_free(&fx.pmm, a + i * PAGE));
    }
    /* Larger than any run: fails without changing anything. */
    SMY_EXPECT_EQ_U64(pmm_alloc_contiguous(&fx.pmm, 2000, 1, &b), PMM_ERR_NO_MEMORY);
    SMY_EXPECT_EQ_U64(pmm_get_stats(&fx.pmm).free_frames, PC_MAP_FREE - 512);
    /* Bad arguments. */
    SMY_EXPECT_EQ_U64(pmm_alloc_contiguous(&fx.pmm, 0, 1, &b), PMM_ERR_BAD_ARGUMENT);
    SMY_EXPECT_EQ_U64(pmm_alloc_contiguous(&fx.pmm, 2, 3, &b), PMM_ERR_BAD_ARGUMENT);
    SMY_EXPECT_EQ_U64(pmm_alloc_contiguous(&fx.pmm, 2, 0, &b), PMM_ERR_BAD_ARGUMENT);
    /* Freeing a range that includes a free frame is rejected as a whole. */
    SMY_EXPECT_EQ_U64(pmm_free_contiguous(&fx.pmm, a, 513), PMM_ERR_DOUBLE_FREE);
    SMY_EXPECT_EQ_U64(pmm_get_stats(&fx.pmm).free_frames, PC_MAP_FREE - 512);
    SMY_EXPECT_EQ_U64(pmm_free_contiguous(&fx.pmm, a, 512), PMM_OK);
    SMY_EXPECT_EQ_U64(pmm_get_stats(&fx.pmm).free_frames, PC_MAP_FREE);
    destroy(&fx);
}

SMY_TEST(pmm_reserved_wins_over_overlapping_usable_and_storage_is_reserved)
{
    /* Overlapping entries (a buggy firmware map): reserved must win, and
     * a partial reserved page reserves the whole frame. */
    const struct pmm_region map[] = {
        {.base = 0x0, .length = 0x100000, .type = PMM_REGION_USABLE},
        {.base = 0x10800, .length = 0x100, .type = PMM_REGION_RESERVED},
    };
    struct pmm pmm;
    uint64_t storage[8];
    uint64_t phys;

    SMY_EXPECT_EQ_U64(pmm_storage_bytes(pmm_frames_to_cover(map, 2)), 64);
    /* Pretend the bitmaps live at 0x20000..0x20040 in this RAM. */
    SMY_EXPECT_EQ_U64(pmm_init(&pmm, map, 2, storage, sizeof storage, 0x20000, 64), PMM_OK);
    SMY_EXPECT(!pmm_is_free(&pmm, 0x10000));
    SMY_EXPECT(!pmm_is_free(&pmm, 0x20000));
    SMY_EXPECT(pmm_is_free(&pmm, 0x21000));
    SMY_EXPECT_EQ_U64(pmm_get_stats(&pmm).free_frames, 256 - 3); /* frame 0, 0x10000, 0x20000 */
    while (pmm_alloc(&pmm, &phys) == PMM_OK) {
        SMY_EXPECT(phys != 0x10000 && phys != 0x20000 && phys != 0);
    }
}

SMY_TEST(pmm_handles_empty_and_degenerate_maps)
{
    const struct pmm_region reserved_only[] = {
        {.base = 0x0, .length = 0x100000, .type = PMM_REGION_RESERVED},
    };
    const struct pmm_region huge[] = {
        {.base = UINT64_MAX - 0xFFF, .length = 0x10000, .type = PMM_REGION_RESERVED},
        {.base = 0x1000, .length = 0, .type = PMM_REGION_USABLE},
    };
    struct pmm pmm;
    uint64_t storage[2];
    uint64_t phys;

    SMY_EXPECT_EQ_U64(pmm_frames_to_cover(reserved_only, 1), 0);
    SMY_EXPECT_EQ_U64(pmm_init(&pmm, reserved_only, 1, storage, sizeof storage, 0, 0), PMM_OK);
    SMY_EXPECT_EQ_U64(pmm_alloc(&pmm, &phys), PMM_ERR_NO_MEMORY);
    SMY_EXPECT_EQ_U64(pmm_free(&pmm, 0x1000), PMM_ERR_BAD_ARGUMENT);
    /* A reserved entry that wraps the address space must not overflow. */
    SMY_EXPECT_EQ_U64(pmm_init(&pmm, huge, 2, storage, sizeof storage, 0, 0), PMM_OK);
    SMY_EXPECT_EQ_U64(pmm_get_stats(&pmm).free_frames, 0);
    /* Storage too small or misaligned is rejected. */
    SMY_EXPECT_EQ_U64(pmm_init(&pmm, pc_map, PC_MAP_COUNT, storage, sizeof storage, 0, 0),
                      PMM_ERR_BAD_ARGUMENT);
    SMY_EXPECT_EQ_U64(pmm_init(&pmm, reserved_only, 1, (char *)storage + 1, 8, 0, 0),
                      PMM_ERR_BAD_ARGUMENT);
}

SMY_TEST(pmm_find_storage_picks_first_fitting_page_aligned_place)
{
    uint64_t phys = 0;

    SMY_EXPECT(pmm_find_storage(pc_map, PC_MAP_COUNT, 0x1000, &phys));
    SMY_EXPECT_EQ_U64(phys, 0x1000); /* skips frame 0 */
    SMY_EXPECT(pmm_find_storage(pc_map, PC_MAP_COUNT, 0x200000, &phys));
    SMY_EXPECT_EQ_U64(phys, 0x100000);
    SMY_EXPECT(!pmm_find_storage(pc_map, PC_MAP_COUNT, 0x800000, &phys));
}

/* xorshift64: deterministic pseudo-random sequence for the stress test. */
static uint64_t next_random(uint64_t *state)
{
    uint64_t x = *state;

    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    *state = x;
    return x;
}

SMY_TEST(pmm_random_alloc_free_matches_a_reference_model)
{
    struct fixture fx = make(pc_map, PC_MAP_COUNT);
    uint64_t *held = calloc(PC_MAP_FREE, sizeof *held);
    size_t held_count = 0;
    uint64_t rng = 0x5eed5eed12345678u;

    SMY_EXPECT(held != NULL);
    if (held == NULL) {
        destroy(&fx);
        return;
    }
    for (int step = 0; step < 20000; step++) {
        const uint64_t r = next_random(&rng);

        if ((r & 3u) != 0 && held_count < PC_MAP_FREE) {
            uint64_t phys;

            if (pmm_alloc(&fx.pmm, &phys) == PMM_OK) {
                held[held_count++] = phys;
            }
        } else if (held_count > 0) {
            const size_t i = (size_t)(r >> 8) % held_count;

            SMY_EXPECT_EQ_U64(pmm_free(&fx.pmm, held[i]), PMM_OK);
            held[i] = held[--held_count];
        }
        if (pmm_get_stats(&fx.pmm).free_frames != PC_MAP_FREE - held_count) {
            SMY_EXPECT_EQ_U64(pmm_get_stats(&fx.pmm).free_frames, PC_MAP_FREE - held_count);
            break;
        }
    }
    /* No frame is held twice. */
    for (size_t i = 0; i < held_count; i++) {
        SMY_EXPECT(!pmm_is_free(&fx.pmm, held[i]));
        for (size_t j = i + 1; j < held_count; j++) {
            if (held[i] == held[j]) {
                SMY_EXPECT(held[i] != held[j]);
            }
        }
    }
    while (held_count > 0) {
        SMY_EXPECT_EQ_U64(pmm_free(&fx.pmm, held[--held_count]), PMM_OK);
    }
    SMY_EXPECT_EQ_U64(pmm_get_stats(&fx.pmm).free_frames, PC_MAP_FREE);
    free(held);
    destroy(&fx);
}

SMY_TEST(pmm_status_names_are_defined)
{
    SMY_EXPECT(strcmp(pmm_status_name(PMM_OK), "ok") == 0);
    SMY_EXPECT(strcmp(pmm_status_name(PMM_ERR_DOUBLE_FREE), "double free") == 0);
}
