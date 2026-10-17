/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

/* ktests for the physical memory manager on the real memory map (Phase 4). */

#include "core/ktest.h"
#include "memory/phys.h"

#define LINK_MAGIC 0x534d592d4f53504du /* "SMY-OSPM" */

/* Each allocated page holds a list node: the previous page and a check
 * value. If a frame were handed out twice, its node would be overwritten
 * and the walk below would find fewer nodes or a broken check value. */
struct page_node {
    uint64_t previous;
    uint64_t check;
};

KTEST(phys_allocates_all_memory_and_frees_without_leak)
{
    const struct pmm_stats before = phys_stats();
    uint64_t head = 0;
    uint64_t allocated = 0;
    uint64_t phys;
    uint64_t walked = 0;
    unsigned outside_usable = 0;
    unsigned bad_nodes = 0;

    KTEST_ASSERT(before.free_frames > 1000);

    while (phys_alloc_page(&phys) == PMM_OK) {
        struct page_node *node = phys_to_virt(phys);

        if (!phys_page_in_usable_region(phys) || phys == 0) {
            outside_usable++;
        }
        node->previous = head;
        node->check = phys ^ LINK_MAGIC;
        head = phys;
        allocated++;
    }
    KTEST_ASSERT_EQ_U64(allocated, before.free_frames);
    KTEST_ASSERT_EQ_U64(phys_stats().free_frames, 0);
    KTEST_ASSERT_EQ_U64(outside_usable, 0);

    while (head != 0) {
        const struct page_node *node = phys_to_virt(head);
        const uint64_t next = node->previous;

        if (node->check != (head ^ LINK_MAGIC)) {
            bad_nodes++;
            break;
        }
        if (phys_free_page(head) != PMM_OK) {
            bad_nodes++;
            break;
        }
        walked++;
        head = next;
    }
    KTEST_ASSERT_EQ_U64(bad_nodes, 0);
    KTEST_ASSERT_EQ_U64(walked, allocated);
    KTEST_ASSERT_EQ_U64(phys_stats().free_frames, before.free_frames);
    KTEST_ASSERT_EQ_U64(phys_stats().used_frames, before.used_frames);
}

KTEST(phys_contiguous_allocation_is_aligned_and_usable)
{
    const struct pmm_stats before = phys_stats();
    uint64_t phys = 0;
    unsigned outside = 0;

    KTEST_ASSERT_EQ_U64(phys_alloc_pages(16, 16, &phys), PMM_OK);
    KTEST_ASSERT_EQ_U64(phys % ((uint64_t)16 * PMM_PAGE_SIZE), 0);
    for (uint64_t i = 0; i < 16; i++) {
        uint64_t *word = phys_to_virt(phys + i * PMM_PAGE_SIZE);

        outside += phys_page_in_usable_region(phys + i * PMM_PAGE_SIZE) ? 0u : 1u;
        *word = i; /* the memory must really be writable RAM */
    }
    KTEST_ASSERT_EQ_U64(outside, 0);
    KTEST_ASSERT_EQ_U64(phys_stats().free_frames, before.free_frames - 16);
    KTEST_ASSERT_EQ_U64(phys_free_pages(phys, 16), PMM_OK);
    KTEST_ASSERT_EQ_U64(phys_stats().free_frames, before.free_frames);
}

KTEST(phys_rejects_double_free_and_foreign_frames)
{
    uint64_t phys = 0;

    KTEST_ASSERT_EQ_U64(phys_alloc_page(&phys), PMM_OK);
    KTEST_ASSERT_EQ_U64(phys_free_page(phys), PMM_OK);
    KTEST_ASSERT_EQ_U64(phys_free_page(phys), PMM_ERR_DOUBLE_FREE);
    KTEST_ASSERT_EQ_U64(phys_free_page(0), PMM_ERR_INVALID_ADDRESS);
    KTEST_ASSERT_EQ_U64(phys_free_page(phys + 1), PMM_ERR_INVALID_ADDRESS);
    KTEST_ASSERT_EQ_U64(phys_free_page(UINT64_MAX & ~(uint64_t)(PMM_PAGE_SIZE - 1)),
                        PMM_ERR_INVALID_ADDRESS);
}
