/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

/* Host tests for kernel/memory/heap.c with a fake page provider. */

#include "harness/smytest.h"
#include "memory/heap.h"

#include <stdlib.h>
#include <string.h>

#define PAGE ((size_t)HEAP_PAGE_SIZE)
#define MAX_RUNS 4096

/* Page runs come from aligned_alloc. Released runs are kept (and poisoned
 * with 0x5A) until teardown, so stale reads stay inside host memory. */
struct provider {
    struct {
        unsigned char *mem;
        size_t pages;
        bool live;
    } runs[MAX_RUNS];
    size_t run_count;
    size_t live_pages;
    size_t page_limit; /* live pages allowed */
};

static void *provider_alloc(void *ctx, size_t count)
{
    struct provider *p = ctx;
    unsigned char *mem;

    if (p->live_pages + count > p->page_limit || p->run_count == MAX_RUNS) {
        return NULL;
    }
    mem = aligned_alloc(PAGE, count * PAGE);
    if (mem == NULL) {
        abort(); /* host out of memory: no point continuing */
    }
    memset(mem, 0xA5, count * PAGE); /* not zeroed, like real pages */
    p->runs[p->run_count].mem = mem;
    p->runs[p->run_count].pages = count;
    p->runs[p->run_count].live = true;
    p->run_count++;
    p->live_pages += count;
    return mem;
}

// cppcheck-suppress constParameterCallback ; signature fixed by struct heap_ops
static void provider_free(void *ctx, void *pages, size_t count)
{
    struct provider *p = ctx;
    bool found = false;

    for (size_t i = 0; i < p->run_count && !found; i++) {
        if (p->runs[i].mem == pages && p->runs[i].live) {
            SMY_EXPECT_EQ_U64(p->runs[i].pages, count);
            p->runs[i].live = false;
            p->live_pages -= count;
            /* Keep the header bytes (as freed RAM keeps its contents); poison the rest. */
            memset(p->runs[i].mem + 64, 0x5A, count * PAGE - 64);
            found = true;
        }
    }
    SMY_EXPECT(found); /* else the heap returned a run it never got */
}

struct fixture {
    struct provider *prov;
    struct heap_ops ops;
    struct heap heap;
};

static void setup(struct fixture *fx, size_t page_limit)
{
    fx->prov = calloc(1, sizeof *fx->prov);
    if (fx->prov == NULL) {
        abort();
    }
    fx->prov->page_limit = page_limit;
    fx->ops = (struct heap_ops){
        .alloc_pages = provider_alloc, .free_pages = provider_free, .ctx = fx->prov};
    heap_init(&fx->heap, &fx->ops);
}

static void teardown(struct fixture *fx)
{
    for (size_t i = 0; i < fx->prov->run_count; i++) {
        free(fx->prov->runs[i].mem);
    }
    free(fx->prov);
}

static void *must_alloc(struct heap *h, size_t size)
{
    void *p = NULL;

    SMY_EXPECT_EQ_U64(heap_alloc(h, size, &p), HEAP_OK);
    SMY_EXPECT(p != NULL);
    return p;
}

SMY_TEST(heap_small_blocks_are_aligned_distinct_and_sized_by_class)
{
    struct fixture fx;
    static const size_t sizes[] = {1, 16, 17, 32, 33, 100, 128, 200, 500, 512, 513, 1000, 1024};
    static const size_t classes[] = {16, 16, 32, 32, 64, 128, 128, 256, 512, 512, 1024, 1024, 1024};
    void *blocks[sizeof sizes / sizeof sizes[0]];

    setup(&fx, 1000);
    for (size_t i = 0; i < sizeof sizes / sizeof sizes[0]; i++) {
        blocks[i] = must_alloc(&fx.heap, sizes[i]);
        SMY_EXPECT_EQ_U64((uintptr_t)blocks[i] % HEAP_ALIGNMENT, 0);
        SMY_EXPECT_EQ_U64(heap_block_size(&fx.heap, blocks[i]), classes[i]);
        memset(blocks[i], (int)i, sizes[i]); /* writable over its whole size (ASan checks) */
    }
    for (size_t i = 0; i < sizeof sizes / sizeof sizes[0]; i++) {
        for (size_t j = i + 1; j < sizeof sizes / sizeof sizes[0]; j++) {
            SMY_EXPECT(blocks[i] != blocks[j]);
        }
    }
    SMY_EXPECT_EQ_U64(heap_get_stats(&fx.heap).live_allocations, 13);
    SMY_EXPECT_EQ_U64(heap_get_stats(&fx.heap).slab_pages, 7); /* one slab per class */
    for (size_t i = 0; i < sizeof sizes / sizeof sizes[0]; i++) {
        SMY_EXPECT_EQ_U64(heap_free(&fx.heap, blocks[i]), HEAP_OK);
    }
    SMY_EXPECT_EQ_U64(heap_get_stats(&fx.heap).live_allocations, 0);
    SMY_EXPECT_EQ_U64(heap_get_stats(&fx.heap).live_bytes, 0);
    teardown(&fx);
}

SMY_TEST(heap_freed_slot_is_poisoned_and_reused)
{
    struct fixture fx;
    unsigned char *a;
    unsigned char *b;

    setup(&fx, 1000);
    a = must_alloc(&fx.heap, 40);
    (void)must_alloc(&fx.heap, 40); /* keeps the slab from being released */
    memset(a, 0x11, 40);
    SMY_EXPECT_EQ_U64(heap_free(&fx.heap, a), HEAP_OK);
    for (size_t i = 0; i < 64; i++) {
        SMY_EXPECT_EQ_U64(a[i], HEAP_POISON);
    }
    SMY_EXPECT_EQ_U64(heap_block_size(&fx.heap, a), 0); /* free slot */
    b = must_alloc(&fx.heap, 64);
    SMY_EXPECT_EQ_PTR(b, a); /* lowest free slot first */
    teardown(&fx);
}

SMY_TEST(heap_large_blocks_use_whole_pages)
{
    struct fixture fx;
    unsigned char *p;

    setup(&fx, 1000);
    p = must_alloc(&fx.heap, 1025);
    SMY_EXPECT_EQ_U64((uintptr_t)p % PAGE, 64); /* header first */
    SMY_EXPECT_EQ_U64(heap_get_stats(&fx.heap).large_pages, 1);
    SMY_EXPECT_EQ_U64(heap_block_size(&fx.heap, p), PAGE - 64);
    SMY_EXPECT_EQ_U64(heap_free(&fx.heap, p), HEAP_OK);
    SMY_EXPECT_EQ_U64(heap_get_stats(&fx.heap).large_pages, 0);

    p = must_alloc(&fx.heap, 3 * PAGE); /* + header = 4 pages */
    SMY_EXPECT_EQ_U64(heap_get_stats(&fx.heap).large_pages, 4);
    memset(p, 0x77, 3 * PAGE);
    SMY_EXPECT_EQ_U64(heap_get_stats(&fx.heap).live_bytes, 3 * PAGE);
    SMY_EXPECT_EQ_U64(heap_free(&fx.heap, p), HEAP_OK);
    SMY_EXPECT_EQ_U64(fx.prov->live_pages, 0);
    teardown(&fx);
}

SMY_TEST(heap_rejects_invalid_pointers_and_double_frees)
{
    struct fixture fx;
    unsigned char *small;
    unsigned char *large;
    unsigned char *foreign = aligned_alloc(PAGE, PAGE);

    setup(&fx, 1000);
    small = must_alloc(&fx.heap, 32);
    (void)must_alloc(&fx.heap, 32); /* keep the slab alive */
    large = must_alloc(&fx.heap, 5000);
    memset(foreign, 0, PAGE);

    SMY_EXPECT_EQ_U64(heap_free(&fx.heap, small + 8), HEAP_ERR_INVALID_POINTER);  /* misaligned */
    SMY_EXPECT_EQ_U64(heap_free(&fx.heap, small + 16), HEAP_ERR_INVALID_POINTER); /* mid-slot */
    SMY_EXPECT_EQ_U64(heap_free(&fx.heap, large + 64), HEAP_ERR_INVALID_POINTER); /* inside */
    SMY_EXPECT_EQ_U64(heap_free(&fx.heap, foreign + 64), HEAP_ERR_INVALID_POINTER);
    SMY_EXPECT_EQ_U64(heap_free(&fx.heap, NULL), HEAP_OK);
    SMY_EXPECT_EQ_U64(heap_get_stats(&fx.heap).live_allocations, 3); /* nothing changed */

    SMY_EXPECT_EQ_U64(heap_free(&fx.heap, small), HEAP_OK);
    SMY_EXPECT_EQ_U64(heap_free(&fx.heap, small), HEAP_ERR_DOUBLE_FREE);
    SMY_EXPECT_EQ_U64(heap_free(&fx.heap, large), HEAP_OK);
    SMY_EXPECT_EQ_U64(heap_free(&fx.heap, large), HEAP_ERR_DOUBLE_FREE);
    SMY_EXPECT_EQ_U64(heap_get_stats(&fx.heap).live_allocations, 1);
    free(foreign);
    teardown(&fx);
}

SMY_TEST(heap_returns_empty_slabs_but_keeps_one_per_class)
{
    struct fixture fx;
    void *p[9];

    setup(&fx, 1000);
    for (int i = 0; i < 9; i++) { /* 1024-byte class: 3 per slab -> 3 slabs */
        p[i] = must_alloc(&fx.heap, 1024);
    }
    SMY_EXPECT_EQ_U64(heap_get_stats(&fx.heap).slab_pages, 3);
    for (int i = 0; i < 9; i++) {
        SMY_EXPECT_EQ_U64(heap_free(&fx.heap, p[i]), HEAP_OK);
    }
    SMY_EXPECT_EQ_U64(heap_get_stats(&fx.heap).slab_pages, 1);
    SMY_EXPECT_EQ_U64(fx.prov->live_pages, 1);
    /* The released slabs' last blocks are reported as double frees. */
    SMY_EXPECT_EQ_U64(heap_free(&fx.heap, p[2]), HEAP_ERR_DOUBLE_FREE);
    teardown(&fx);
}

SMY_TEST(heap_out_of_memory_and_bad_sizes_change_nothing)
{
    struct fixture fx;
    void *p = NULL;

    setup(&fx, 2);
    SMY_EXPECT_EQ_U64(heap_alloc(&fx.heap, 0, &p), HEAP_ERR_BAD_ARGUMENT);
    SMY_EXPECT_EQ_U64(heap_alloc(&fx.heap, HEAP_MAX_ALLOCATION + 1, &p), HEAP_ERR_BAD_ARGUMENT);
    SMY_EXPECT_EQ_U64(heap_alloc(&fx.heap, 3 * PAGE, &p), HEAP_ERR_NO_MEMORY);
    (void)must_alloc(&fx.heap, 16);
    (void)must_alloc(&fx.heap, 2000);
    SMY_EXPECT_EQ_U64(heap_alloc(&fx.heap, 32, &p), HEAP_ERR_NO_MEMORY);
    SMY_EXPECT_EQ_U64(heap_get_stats(&fx.heap).live_allocations, 2);
    SMY_EXPECT(heap_get_stats(&fx.heap).slab_pages == 1);
    SMY_EXPECT(strcmp(heap_status_name(HEAP_ERR_DOUBLE_FREE), "double free") == 0);
    teardown(&fx);
}

/* Random alloc/free against a reference list; every block is filled with a
 * pattern derived from its id and checked when it is freed. */
SMY_TEST(heap_random_alloc_free_keeps_contents_and_accounting)
{
    enum { SLOTS = 600, STEPS = 30000 };
    struct fixture fx;
    static struct {
        unsigned char *p;
        size_t size;
        unsigned id;
    } live[SLOTS];
    unsigned seed = 12345;
    unsigned next_id = 1;
    uint64_t live_count = 0;

    memset(live, 0, sizeof live);
    setup(&fx, 100000);
    for (int step = 0; step < STEPS; step++) {
        unsigned slot;

        seed = seed * 1103515245u + 12345u;
        slot = (seed >> 8) % SLOTS;
        if (live[slot].p == NULL) {
            /* Mostly small, sometimes large. */
            const size_t size =
                ((seed >> 4) % 8 == 0) ? 1025 + (seed >> 12) % 9000 : 1 + (seed >> 12) % 1024;

            live[slot].p = must_alloc(&fx.heap, size);
            live[slot].size = size;
            live[slot].id = next_id++;
            memset(live[slot].p, (int)(live[slot].id & 0xFF), size);
            live_count++;
        } else {
            bool intact = true;

            for (size_t i = 0; i < live[slot].size && intact; i++) {
                intact = live[slot].p[i] == (unsigned char)(live[slot].id & 0xFF);
            }
            SMY_EXPECT(intact); /* contents unchanged while the block was live */
            SMY_EXPECT_EQ_U64(heap_free(&fx.heap, live[slot].p), HEAP_OK);
            live[slot].p = NULL;
            live_count--;
        }
        if (heap_get_stats(&fx.heap).live_allocations != live_count) {
            SMY_EXPECT_EQ_U64(heap_get_stats(&fx.heap).live_allocations, live_count);
            break;
        }
    }
    for (unsigned s = 0; s < SLOTS; s++) {
        if (live[s].p != NULL) {
            SMY_EXPECT_EQ_U64(heap_free(&fx.heap, live[s].p), HEAP_OK);
        }
    }
    SMY_EXPECT_EQ_U64(heap_get_stats(&fx.heap).live_allocations, 0);
    SMY_EXPECT_EQ_U64(heap_get_stats(&fx.heap).live_bytes, 0);
    SMY_EXPECT_EQ_U64(heap_get_stats(&fx.heap).large_pages, 0);
    SMY_EXPECT(heap_get_stats(&fx.heap).slab_pages <= HEAP_CLASS_COUNT);
    SMY_EXPECT_EQ_U64(fx.prov->live_pages, heap_get_stats(&fx.heap).slab_pages);
    teardown(&fx);
}
