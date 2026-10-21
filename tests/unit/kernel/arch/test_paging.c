/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

/* Host tests for kernel/arch/x86_64/paging.c on a fake pool of table pages. */

#include "arch/x86_64/paging.h"
#include "harness/smytest.h"

#include <stdlib.h>
#include <string.h>

#define POOL_PAGES 64u
#define KIB4 ((uint64_t)PAGING_PAGE_SIZE)
#define MIB2 ((uint64_t)PAGING_LARGE_PAGE_SIZE)
#define HIGH 0xffff800000000000ull
#define KERNEL 0xffffffff80000000ull

/* Table pages live in a host buffer; "physical" address = (index + 1) * 4 KiB
 * so that 0 is never a valid table. */
struct pool {
    unsigned char *mem;
    unsigned used;
    unsigned limit; /* allocations allowed before reporting out of memory */
    unsigned invalidations;
    uint64_t last_invalidated;
};

static bool pool_alloc(void *ctx, uint64_t *phys_out)
{
    struct pool *p = ctx;

    if (p->used >= p->limit) {
        return false;
    }
    memset(p->mem + (size_t)p->used * KIB4, 0xAB, KIB4); /* garbage: paging.c must zero */
    *phys_out = (uint64_t)(p->used + 1) * KIB4;
    p->used++;
    return true;
}

static void *pool_ptr(void *ctx, uint64_t phys)
{
    const struct pool *p = ctx;
    const uint64_t index = phys / KIB4 - 1;

    SMY_EXPECT(phys % KIB4 == 0 && index < p->used);
    return p->mem + index * KIB4;
}

static void pool_invalidate(void *ctx, uint64_t virt)
{
    struct pool *p = ctx;

    p->invalidations++;
    p->last_invalidated = virt;
}

struct fixture {
    struct pool pool;
    struct paging_ops ops;
    struct address_space as;
};

static void setup(struct fixture *fx, unsigned limit)
{
    fx->pool = (struct pool){.mem = aligned_alloc(KIB4, POOL_PAGES * KIB4), .limit = limit};
    SMY_EXPECT(fx->pool.mem != NULL);
    fx->ops = (struct paging_ops){
        .alloc_table = pool_alloc,
        .table_ptr = pool_ptr,
        .invalidate = pool_invalidate,
        .ctx = &fx->pool,
    };
    SMY_EXPECT_EQ_U64(paging_init(&fx->as, &fx->ops), VM_OK);
}

static void teardown(struct fixture *fx)
{
    free(fx->pool.mem);
}

SMY_TEST(paging_map_translate_and_hardware_bits)
{
    struct fixture fx;
    struct vm_translation t;
    struct vm_walk w;

    setup(&fx, POOL_PAGES);
    SMY_EXPECT_EQ_U64(fx.as.table_pages, 1);
    SMY_EXPECT_EQ_U64(paging_map(&fx.as, KERNEL + 0x5000, 0x123000, VM_WRITE), VM_OK);
    SMY_EXPECT_EQ_U64(fx.as.table_pages, 4); /* PML4 + PDPT + PD + PT */

    SMY_EXPECT_EQ_U64(paging_translate(&fx.as, KERNEL + 0x5ABC, &t), VM_OK);
    SMY_EXPECT_EQ_U64(t.phys, 0x123ABC);
    SMY_EXPECT_EQ_U64(t.flags, VM_WRITE);
    SMY_EXPECT_EQ_U64(t.page_size, KIB4);

    paging_walk(&fx.as, KERNEL + 0x5000, &w);
    SMY_EXPECT_EQ_U64(w.levels, 4);
    /* Leaf: present, writable, not user, no-execute. */
    SMY_EXPECT_EQ_U64(w.entries[3], 0x123000 | PTE_PRESENT | PTE_WRITABLE | PTE_NO_EXECUTE);
    /* Intermediate: present + writable, executable (the leaf decides). */
    SMY_EXPECT_EQ_U64(w.entries[0] & ~PTE_ADDRESS_MASK, PTE_PRESENT | PTE_WRITABLE);

    SMY_EXPECT_EQ_U64(paging_map(&fx.as, KERNEL + 0x6000, 0x7000, VM_EXEC), VM_OK);
    SMY_EXPECT_EQ_U64(fx.as.table_pages, 4); /* same PT reused */
    SMY_EXPECT_EQ_U64(paging_translate(&fx.as, KERNEL + 0x6000, &t), VM_OK);
    SMY_EXPECT_EQ_U64(t.flags, VM_EXEC);
    paging_walk(&fx.as, KERNEL + 0x6000, &w);
    SMY_EXPECT_EQ_U64(w.entries[3], 0x7000 | PTE_PRESENT); /* read-only, executable */

    SMY_EXPECT_EQ_U64(paging_translate(&fx.as, KERNEL + 0x7000, &t), VM_ERR_NOT_MAPPED);
    SMY_EXPECT_EQ_U64(paging_translate(&fx.as, HIGH, &t), VM_ERR_NOT_MAPPED);
    teardown(&fx);
}

SMY_TEST(paging_attribute_and_user_bits)
{
    struct fixture fx;
    struct vm_walk w;
    struct vm_translation t;

    setup(&fx, POOL_PAGES);
    SMY_EXPECT_EQ_U64(paging_map(&fx.as, HIGH, 0x1000, VM_WRITE | VM_WRITE_COMBINING), VM_OK);
    paging_walk(&fx.as, HIGH, &w);
    SMY_EXPECT_EQ_U64(w.entries[3] & (PTE_PWT | PTE_PCD), PTE_PWT);
    SMY_EXPECT_EQ_U64(paging_map(&fx.as, HIGH + KIB4, 0x2000, VM_UNCACHED), VM_OK);
    paging_walk(&fx.as, HIGH + KIB4, &w);
    SMY_EXPECT_EQ_U64(w.entries[3] & (PTE_PWT | PTE_PCD), PTE_PCD);

    SMY_EXPECT_EQ_U64(paging_map(&fx.as, 0x400000, 0x3000, VM_USER | VM_WRITE), VM_OK);
    paging_walk(&fx.as, 0x400000, &w);
    for (unsigned i = 0; i < 4; i++) {
        SMY_EXPECT((w.entries[i] & PTE_USER) != 0); /* user needs U on every level */
    }
    SMY_EXPECT_EQ_U64(paging_translate(&fx.as, 0x400000, &t), VM_OK);
    SMY_EXPECT_EQ_U64(t.flags, VM_USER | VM_WRITE);
    teardown(&fx);
}

SMY_TEST(paging_rejects_bad_arguments)
{
    struct fixture fx;

    setup(&fx, POOL_PAGES);
    SMY_EXPECT_EQ_U64(paging_map(&fx.as, KERNEL + 1, 0x1000, 0), VM_ERR_BAD_ARGUMENT);
    SMY_EXPECT_EQ_U64(paging_map(&fx.as, KERNEL, 0x1001, 0), VM_ERR_BAD_ARGUMENT);
    SMY_EXPECT_EQ_U64(paging_map(&fx.as, 0x0000800000000000ull, 0x1000, 0), VM_ERR_BAD_ARGUMENT);
    SMY_EXPECT_EQ_U64(paging_map(&fx.as, KERNEL, 1ull << 52, 0), VM_ERR_BAD_ARGUMENT);
    SMY_EXPECT_EQ_U64(paging_map(&fx.as, KERNEL, 0x1000, 1u << 9), VM_ERR_BAD_ARGUMENT);
    SMY_EXPECT_EQ_U64(paging_map(&fx.as, KERNEL, 0x1000, VM_WRITE_COMBINING | VM_UNCACHED),
                      VM_ERR_BAD_ARGUMENT);
    SMY_EXPECT_EQ_U64(paging_map_large(&fx.as, KERNEL + KIB4, 0, 0), VM_ERR_BAD_ARGUMENT);
    SMY_EXPECT_EQ_U64(paging_map_large(&fx.as, KERNEL, KIB4, 0), VM_ERR_BAD_ARGUMENT);
    SMY_EXPECT_EQ_U64(paging_unmap(&fx.as, KERNEL + 8, NULL), VM_ERR_BAD_ARGUMENT);
    SMY_EXPECT_EQ_U64(paging_map_range(&fx.as, KERNEL, 0, KIB4 + 1, 0), VM_ERR_BAD_ARGUMENT);
    SMY_EXPECT_EQ_U64(fx.as.table_pages, 1); /* nothing was built */
    SMY_EXPECT(paging_is_canonical(0x00007FFFFFFFF000ull));
    SMY_EXPECT(paging_is_canonical(HIGH));
    SMY_EXPECT(!paging_is_canonical(0x8000000000000000ull));
    teardown(&fx);
}

SMY_TEST(paging_refuses_to_map_twice_and_unmaps)
{
    struct fixture fx;
    struct vm_translation t;
    uint64_t phys = 0;

    setup(&fx, POOL_PAGES);
    SMY_EXPECT_EQ_U64(paging_map(&fx.as, KERNEL, 0x1000, VM_WRITE), VM_OK);
    SMY_EXPECT_EQ_U64(paging_map(&fx.as, KERNEL, 0x2000, VM_WRITE), VM_ERR_ALREADY_MAPPED);
    SMY_EXPECT_EQ_U64(paging_translate(&fx.as, KERNEL, &t), VM_OK);
    SMY_EXPECT_EQ_U64(t.phys, 0x1000); /* unchanged */

    SMY_EXPECT_EQ_U64(paging_unmap(&fx.as, KERNEL, &phys), VM_OK);
    SMY_EXPECT_EQ_U64(phys, 0x1000);
    SMY_EXPECT_EQ_U64(fx.pool.invalidations, 1);
    SMY_EXPECT_EQ_U64(fx.pool.last_invalidated, KERNEL);
    SMY_EXPECT_EQ_U64(paging_translate(&fx.as, KERNEL, &t), VM_ERR_NOT_MAPPED);
    SMY_EXPECT_EQ_U64(paging_unmap(&fx.as, KERNEL, &phys), VM_ERR_NOT_MAPPED);
    SMY_EXPECT_EQ_U64(paging_unmap(&fx.as, HIGH, &phys), VM_ERR_NOT_MAPPED); /* no tables */
    SMY_EXPECT_EQ_U64(paging_map(&fx.as, KERNEL, 0x2000, 0), VM_OK);         /* reusable */
    teardown(&fx);
}

SMY_TEST(paging_protect_changes_rights_and_invalidates)
{
    struct fixture fx;
    struct vm_translation t;

    setup(&fx, POOL_PAGES);
    SMY_EXPECT_EQ_U64(paging_map(&fx.as, KERNEL, 0x9000, VM_WRITE), VM_OK);
    SMY_EXPECT_EQ_U64(paging_protect(&fx.as, KERNEL, VM_EXEC), VM_OK);
    SMY_EXPECT_EQ_U64(paging_translate(&fx.as, KERNEL, &t), VM_OK);
    SMY_EXPECT_EQ_U64(t.flags, VM_EXEC);
    SMY_EXPECT_EQ_U64(t.phys, 0x9000);
    SMY_EXPECT_EQ_U64(fx.pool.invalidations, 1);
    SMY_EXPECT_EQ_U64(paging_protect(&fx.as, KERNEL + KIB4, 0), VM_ERR_NOT_MAPPED);
    SMY_EXPECT_EQ_U64(paging_protect(&fx.as, KERNEL, 1u << 9), VM_ERR_BAD_ARGUMENT);
    teardown(&fx);
}

SMY_TEST(paging_large_pages_translate_and_block_small_ones)
{
    struct fixture fx;
    struct vm_translation t;
    struct vm_walk w;

    setup(&fx, POOL_PAGES);
    SMY_EXPECT_EQ_U64(paging_map_large(&fx.as, HIGH + MIB2, 0x40000000, VM_WRITE), VM_OK);
    SMY_EXPECT_EQ_U64(fx.as.table_pages, 3); /* no PT */
    SMY_EXPECT_EQ_U64(paging_translate(&fx.as, HIGH + MIB2 + 0x12345, &t), VM_OK);
    SMY_EXPECT_EQ_U64(t.phys, 0x40012345);
    SMY_EXPECT_EQ_U64(t.page_size, MIB2);
    SMY_EXPECT_EQ_U64(t.flags, VM_WRITE);
    paging_walk(&fx.as, HIGH + MIB2, &w);
    SMY_EXPECT_EQ_U64(w.levels, 3);
    SMY_EXPECT((w.entries[2] & PTE_LARGE) != 0);

    SMY_EXPECT_EQ_U64(paging_map(&fx.as, HIGH + MIB2 + KIB4, 0x1000, 0), VM_ERR_LARGE_PAGE);
    SMY_EXPECT_EQ_U64(paging_unmap(&fx.as, HIGH + MIB2, NULL), VM_ERR_LARGE_PAGE);
    SMY_EXPECT_EQ_U64(paging_map_large(&fx.as, HIGH + MIB2, 0, 0), VM_ERR_ALREADY_MAPPED);
    /* A 4 KiB page next door shares the PD but gets its own PT. */
    SMY_EXPECT_EQ_U64(paging_map(&fx.as, HIGH, 0x1000, 0), VM_OK);
    SMY_EXPECT_EQ_U64(paging_map_large(&fx.as, HIGH, 0, 0), VM_ERR_ALREADY_MAPPED);
    teardown(&fx);
}

SMY_TEST(paging_map_range_mixes_page_sizes)
{
    struct fixture fx;
    struct vm_translation t;
    /* 4 KiB below the 2 MiB boundary, then 2 large pages, then 3 small. */
    const uint64_t virt = HIGH + MIB2 - KIB4;
    const uint64_t phys = 0x10000000 + MIB2 - KIB4;
    const uint64_t size = KIB4 + 2 * MIB2 + 3 * KIB4;

    setup(&fx, POOL_PAGES);
    SMY_EXPECT_EQ_U64(paging_map_range(&fx.as, virt, phys, size, VM_WRITE), VM_OK);
    for (uint64_t off = 0; off < size; off += KIB4) {
        SMY_EXPECT_EQ_U64(paging_translate(&fx.as, virt + off, &t), VM_OK);
        SMY_EXPECT_EQ_U64(t.phys, phys + off);
    }
    SMY_EXPECT_EQ_U64(paging_translate(&fx.as, virt + KIB4, &t), VM_OK);
    SMY_EXPECT_EQ_U64(t.page_size, MIB2);
    SMY_EXPECT_EQ_U64(paging_translate(&fx.as, virt + KIB4 + 2 * MIB2, &t), VM_OK);
    SMY_EXPECT_EQ_U64(t.page_size, KIB4);
    SMY_EXPECT_EQ_U64(paging_translate(&fx.as, virt + size, &t), VM_ERR_NOT_MAPPED);
    SMY_EXPECT_EQ_U64(paging_translate(&fx.as, virt - KIB4, &t), VM_ERR_NOT_MAPPED);

    /* Misaligned against each other: all 4 KiB pages. */
    SMY_EXPECT_EQ_U64(paging_map_range(&fx.as, KERNEL, KIB4, MIB2, 0), VM_OK);
    SMY_EXPECT_EQ_U64(paging_translate(&fx.as, KERNEL, &t), VM_OK);
    SMY_EXPECT_EQ_U64(t.page_size, KIB4);
    teardown(&fx);
}

SMY_TEST(paging_reports_out_of_memory_without_corruption)
{
    struct fixture fx;
    struct vm_translation t;

    setup(&fx, 3); /* root + 2: one table short of a full path */
    SMY_EXPECT_EQ_U64(paging_map(&fx.as, KERNEL, 0x1000, 0), VM_ERR_NO_MEMORY);
    SMY_EXPECT_EQ_U64(paging_translate(&fx.as, KERNEL, &t), VM_ERR_NOT_MAPPED);
    fx.pool.limit = POOL_PAGES;
    SMY_EXPECT_EQ_U64(paging_map(&fx.as, KERNEL, 0x1000, 0), VM_OK); /* completes the path */
    SMY_EXPECT_EQ_U64(fx.as.table_pages, 4);
    teardown(&fx);
}

SMY_TEST(paging_status_names_are_defined)
{
    for (int s = VM_OK; s <= VM_ERR_LARGE_PAGE; s++) {
        SMY_EXPECT(strcmp(vm_status_name((enum vm_status)s), "unknown") != 0);
    }
}
