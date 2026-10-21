/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

#include "memory/kmalloc.h"

#include "core/panic.h"
#include "core/string.h"
#include "memory/phys.h"

static void *heap_pages_alloc(void *ctx, size_t count)
{
    uint64_t phys;

    (void)ctx;
    if (phys_alloc_pages(count, 1, &phys) != PMM_OK) {
        return NULL;
    }
    return phys_to_virt(phys);
}

static void heap_pages_free(void *ctx, void *pages, size_t count)
{
    const uint64_t phys = phys_from_virt(pages);
    const enum pmm_status st = phys_free_pages(phys, count);

    (void)ctx;
    if (st != PMM_OK) {
        panic("kmalloc: returning %zu pages at %p failed: %s", count, pages, pmm_status_name(st));
    }
}

static const struct heap_ops kernel_heap_ops = {
    .alloc_pages = heap_pages_alloc,
    .free_pages = heap_pages_free,
    .ctx = NULL,
};

static struct heap kernel_heap;

void kmalloc_init(void)
{
    heap_init(&kernel_heap, &kernel_heap_ops);
}

void *kmalloc(size_t size)
{
    void *p = NULL;

    return heap_alloc(&kernel_heap, size, &p) == HEAP_OK ? p : NULL;
}

void *kzalloc(size_t size)
{
    void *p = kmalloc(size);

    if (p != NULL) {
        memset(p, 0, size);
    }
    return p;
}

void kfree(void *ptr)
{
    const enum heap_status st = heap_free(&kernel_heap, ptr);

    if (st != HEAP_OK) {
        panic("kfree(%p): %s", ptr, heap_status_name(st));
    }
}

struct heap_stats kmalloc_stats(void)
{
    return heap_get_stats(&kernel_heap);
}
