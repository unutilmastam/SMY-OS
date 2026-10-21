/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

/* ktests for the kernel heap (Phase 5, docs/kernel/memory.md). */

#include "core/ktest.h"
#include "memory/kmalloc.h"
#include "memory/phys.h"

#define SLOTS 512
#define STEPS 20000

static struct {
    uint8_t *p;
    size_t size;
    uint8_t tag;
} live[SLOTS];

/* Random kmalloc/kfree with contents checked: blocks never overlap and
 * everything returns to the page allocator except cached empty slabs. */
KTEST(heap_stress_random_alloc_free_without_leak)
{
    const uint64_t free_before = phys_stats().free_frames;
    const uint64_t slabs_before = kmalloc_stats().slab_pages;
    uint32_t seed = 0x5EED;
    uint8_t next_tag = 1;

    for (int step = 0; step < STEPS; step++) {
        unsigned slot;

        seed = seed * 1664525u + 1013904223u;
        slot = (seed >> 10) % SLOTS;
        if (live[slot].p == NULL) {
            const size_t size =
                ((seed >> 4) % 16 == 0) ? 1025 + (seed >> 12) % 20000 : 1 + (seed >> 12) % 1024;

            live[slot].p = kmalloc(size);
            if (live[slot].p == NULL) {
                KTEST_ASSERT(live[slot].p != NULL);
                return;
            }
            KTEST_ASSERT_EQ_U64((uintptr_t)live[slot].p % 16, 0);
            live[slot].size = size;
            live[slot].tag = next_tag++;
            for (size_t i = 0; i < size; i++) {
                live[slot].p[i] = live[slot].tag;
            }
        } else {
            for (size_t i = 0; i < live[slot].size; i++) {
                if (live[slot].p[i] != live[slot].tag) {
                    ktest_fail(__FILE__, __LINE__, "block %p overwritten while live",
                               (void *)live[slot].p);
                    return;
                }
            }
            kfree(live[slot].p);
            live[slot].p = NULL;
        }
    }
    for (unsigned s = 0; s < SLOTS; s++) {
        kfree(live[s].p);
        live[s].p = NULL;
    }
    KTEST_ASSERT_EQ_U64(kmalloc_stats().live_allocations, 0);
    KTEST_ASSERT_EQ_U64(kmalloc_stats().large_pages, 0);
    KTEST_ASSERT_EQ_U64(free_before - phys_stats().free_frames,
                        kmalloc_stats().slab_pages - slabs_before);
}

KTEST(heap_large_block_spans_pages_and_is_returned)
{
    const uint64_t free_before = phys_stats().free_frames;
    uint8_t *p = kmalloc(1u << 20);

    if (p == NULL) {
        KTEST_ASSERT(p != NULL);
        return;
    }
    for (size_t i = 0; i < (1u << 20); i++) {
        p[i] = (uint8_t)i;
    }
    KTEST_ASSERT_EQ_U64(p[(1u << 20) - 1], 0xFF);
    KTEST_ASSERT_EQ_U64(free_before - phys_stats().free_frames, 257); /* 1 MiB + header page */
    kfree(p);
    KTEST_ASSERT_EQ_U64(phys_stats().free_frames, free_before);
}

KTEST(heap_kzalloc_zeroes_and_size_zero_fails)
{
    uint8_t *p = kmalloc(200);
    uint8_t *z;

    if (p == NULL) {
        KTEST_ASSERT(p != NULL);
        return;
    }
    for (int i = 0; i < 200; i++) {
        p[i] = 0xEE;
    }
    kfree(p);
    z = kzalloc(200); /* likely the same slot, now poisoned */
    if (z == NULL) {
        KTEST_ASSERT(z != NULL);
        return;
    }
    for (int i = 0; i < 200; i++) {
        KTEST_ASSERT_EQ_U64(z[i], 0);
    }
    kfree(z);
    KTEST_ASSERT(kmalloc(0) == NULL);
}

/* Must panic with "kfree(...): double free". */
KTEST_PANIC(panic_on_double_kfree)
{
    void *p = kmalloc(64);
    void *keep = kmalloc(64); /* keeps the slab alive */

    kfree(p);
    kfree(p); // NOLINT(clang-analyzer-unix.Malloc): the double free under test
    kfree(keep);
}
