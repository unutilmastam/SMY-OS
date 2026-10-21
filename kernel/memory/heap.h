/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

/*
 * Kernel heap core: size-class slabs for small objects, whole pages for
 * large ones (docs/kernel/memory.md, ADR 0012). Pure logic, host-testable:
 * pages come from and go back to `struct heap_ops`.
 *
 * Small (1..1024 bytes): seven classes 16, 32, ..., 1024. A slab is one
 * 4 KiB page: a 64-byte header (magic, class, free bitmap, list links)
 * followed by equal slots. Large (> 1024 bytes): whole pages with the same
 * 64-byte header in the first page. So every pointer the heap returns is
 * 16-byte aligned and its header is at (pointer & ~4095).
 *
 * Checks: a pointer whose page has no heap magic, that is not at a slot
 * boundary, or that is already free is rejected (heap_free returns an error;
 * the kernel wrapper panics). Pages given back are marked "DEAD", so a second
 * free of a large block or of the last block of a released slab is still
 * reported as a double free. Freed slots are filled with 0xDF to make
 * use-after-free visible. Limit: a stale pointer into a page that was given
 * back and has been reused can pass the checks.
 *
 * Not thread-safe: callers serialise access (single CPU until Phase 16).
 */

#ifndef SMY_KERNEL_MEMORY_HEAP_H
#define SMY_KERNEL_MEMORY_HEAP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define HEAP_PAGE_SIZE 4096u
#define HEAP_CLASS_COUNT 7u
#define HEAP_SMALL_MAX 1024u
#define HEAP_MAX_ALLOCATION ((size_t)1 << 30) /* 1 GiB: larger requests are bugs */
#define HEAP_ALIGNMENT 16u
#define HEAP_POISON 0xDF

struct heap_ops {
    /* `count` physically contiguous pages, mapped and writable; NULL if none. */
    void *(*alloc_pages)(void *ctx, size_t count);
    void (*free_pages)(void *ctx, void *pages, size_t count);
    void *ctx;
};

struct heap_slab; /* page header, heap.c */

struct heap_stats {
    uint64_t live_allocations;
    uint64_t live_bytes;  /* small: class size, large: bytes requested */
    uint64_t slab_pages;  /* pages held for small objects (incl. cached empty slabs) */
    uint64_t large_pages; /* pages held for large objects */
};

struct heap {
    const struct heap_ops *ops;
    struct heap_slab *partial[HEAP_CLASS_COUNT]; /* slabs with a free slot */
    struct heap_stats stats;
};

enum heap_status {
    HEAP_OK = 0,
    HEAP_ERR_NO_MEMORY,
    HEAP_ERR_BAD_ARGUMENT,    /* size 0 or > HEAP_MAX_ALLOCATION */
    HEAP_ERR_INVALID_POINTER, /* not a block this heap handed out */
    HEAP_ERR_DOUBLE_FREE,
};

void heap_init(struct heap *heap, const struct heap_ops *ops);

enum heap_status heap_alloc(struct heap *heap, size_t size, void **out);

/* Free a block; NULL is accepted and ignored. */
enum heap_status heap_free(struct heap *heap, void *ptr);

/* Usable size of a live block (at least the requested size), 0 if invalid. */
size_t heap_block_size(const struct heap *heap, const void *ptr);

struct heap_stats heap_get_stats(const struct heap *heap);

const char *heap_status_name(enum heap_status status);

#endif /* SMY_KERNEL_MEMORY_HEAP_H */
