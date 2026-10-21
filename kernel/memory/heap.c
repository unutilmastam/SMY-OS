/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

#include "memory/heap.h"

#define HEADER_SIZE 64u
#define SLAB_MAGIC 0x534C4142u  /* "SLAB" */
#define LARGE_MAGIC 0x4C415247u /* "LARG" */
#define DEAD_MAGIC 0x44454144u  /* "DEAD": a page run the heap gave back */
#define BITMAP_WORDS 4u         /* 256 slots: enough for the 16-byte class (252) */

/* The first 64 bytes of every heap page run. */
struct heap_slab {
    uint32_t magic;
    uint16_t class_index;   /* small: size class; large: unused */
    uint16_t capacity;      /* small: slots in this slab */
    uint32_t free_count;    /* small: free slots */
    uint32_t pages;         /* large: pages in the run */
    struct heap_slab *next; /* small: partial-list links */
    struct heap_slab *prev;
    union {
        uint64_t free_map[BITMAP_WORDS]; /* small: 1 = slot free */
        uint64_t requested;              /* large: bytes requested */
    } u;
};

_Static_assert(sizeof(struct heap_slab) <= HEADER_SIZE, "slab header must fit in 64 bytes");

static const uint32_t class_sizes[HEAP_CLASS_COUNT] = {16, 32, 64, 128, 256, 512, 1024};

static uint32_t slot_capacity(unsigned class_index)
{
    return (HEAP_PAGE_SIZE - HEADER_SIZE) / class_sizes[class_index];
}

static unsigned class_for(size_t size)
{
    unsigned c = 0;

    while (class_sizes[c] < size) {
        c++;
    }
    return c;
}

static unsigned char *slot_ptr(struct heap_slab *slab, unsigned slot)
{
    return (unsigned char *)slab + HEADER_SIZE + (size_t)slot * class_sizes[slab->class_index];
}

static void fill(void *p, unsigned char value, size_t n)
{
    unsigned char *b = p;

    for (size_t i = 0; i < n; i++) {
        b[i] = value;
    }
}

void heap_init(struct heap *heap, const struct heap_ops *ops)
{
    heap->ops = ops;
    for (unsigned c = 0; c < HEAP_CLASS_COUNT; c++) {
        heap->partial[c] = NULL;
    }
    heap->stats = (struct heap_stats){0};
}

static void list_push(struct heap *heap, struct heap_slab *slab)
{
    struct heap_slab **head = &heap->partial[slab->class_index];

    slab->prev = NULL;
    slab->next = *head;
    if (*head != NULL) {
        (*head)->prev = slab;
    }
    *head = slab;
}

static void list_remove(struct heap *heap, struct heap_slab *slab)
{
    if (slab->prev != NULL) {
        slab->prev->next = slab->next;
    } else {
        heap->partial[slab->class_index] = slab->next;
    }
    if (slab->next != NULL) {
        slab->next->prev = slab->prev;
    }
    slab->next = NULL;
    slab->prev = NULL;
}

static struct heap_slab *new_slab(struct heap *heap, unsigned class_index)
{
    struct heap_slab *slab = heap->ops->alloc_pages(heap->ops->ctx, 1);
    const uint32_t capacity = slot_capacity(class_index);

    if (slab == NULL) {
        return NULL;
    }
    *slab = (struct heap_slab){
        .magic = SLAB_MAGIC,
        .class_index = (uint16_t)class_index,
        .capacity = (uint16_t)capacity,
        .free_count = capacity,
    };
    for (uint32_t s = 0; s < capacity; s++) {
        slab->u.free_map[s / 64] |= 1ull << (s % 64);
    }
    heap->stats.slab_pages++;
    list_push(heap, slab);
    return slab;
}

static enum heap_status alloc_small(struct heap *heap, size_t size, void **out)
{
    const unsigned c = class_for(size);
    struct heap_slab *slab = heap->partial[c];

    if (slab == NULL) {
        slab = new_slab(heap, c);
        if (slab == NULL) {
            return HEAP_ERR_NO_MEMORY;
        }
    }
    for (unsigned w = 0; w < BITMAP_WORDS; w++) {
        if (slab->u.free_map[w] != 0) {
            const unsigned bit = (unsigned)__builtin_ctzll(slab->u.free_map[w]);

            slab->u.free_map[w] &= ~(1ull << bit);
            slab->free_count--;
            if (slab->free_count == 0) {
                list_remove(heap, slab);
            }
            *out = slot_ptr(slab, w * 64 + bit);
            return HEAP_OK;
        }
    }
    return HEAP_ERR_NO_MEMORY; /* not reached: a partial slab has a free bit */
}

static enum heap_status alloc_large(struct heap *heap, size_t size, void **out)
{
    const size_t pages = (size + HEADER_SIZE + HEAP_PAGE_SIZE - 1) / HEAP_PAGE_SIZE;
    struct heap_slab *run = heap->ops->alloc_pages(heap->ops->ctx, pages);

    if (run == NULL) {
        return HEAP_ERR_NO_MEMORY;
    }
    *run = (struct heap_slab){.magic = LARGE_MAGIC, .pages = (uint32_t)pages, .u.requested = size};
    heap->stats.large_pages += pages;
    *out = (unsigned char *)run + HEADER_SIZE;
    return HEAP_OK;
}

enum heap_status heap_alloc(struct heap *heap, size_t size, void **out)
{
    enum heap_status st;

    if (size == 0 || size > HEAP_MAX_ALLOCATION) {
        return HEAP_ERR_BAD_ARGUMENT;
    }
    st = size <= HEAP_SMALL_MAX ? alloc_small(heap, size, out) : alloc_large(heap, size, out);
    if (st == HEAP_OK) {
        heap->stats.live_allocations++;
        heap->stats.live_bytes += size <= HEAP_SMALL_MAX ? class_sizes[class_for(size)] : size;
    }
    return st;
}

static struct heap_slab *header_of(const void *ptr)
{
    return (struct heap_slab *)((uintptr_t)ptr & ~(uintptr_t)(HEAP_PAGE_SIZE - 1));
}

/* Slot index of `ptr` in `slab`, or -1 if it is not exactly at a slot. */
static long slot_of(const struct heap_slab *slab, const void *ptr)
{
    const uintptr_t offset = (uintptr_t)ptr - (uintptr_t)slab;
    const uint32_t size = slab->class_index < HEAP_CLASS_COUNT ? class_sizes[slab->class_index] : 0;

    if (size == 0 || offset < HEADER_SIZE || (offset - HEADER_SIZE) % size != 0 ||
        (offset - HEADER_SIZE) / size >= slab->capacity) {
        return -1;
    }
    return (long)((offset - HEADER_SIZE) / size);
}

static enum heap_status free_small(struct heap *heap, struct heap_slab *slab, void *ptr)
{
    const long slot = slot_of(slab, ptr);
    uint64_t bit;

    if (slot < 0) {
        return HEAP_ERR_INVALID_POINTER;
    }
    bit = 1ull << ((unsigned long)slot % 64);
    if ((slab->u.free_map[slot / 64] & bit) != 0) {
        return HEAP_ERR_DOUBLE_FREE;
    }
    fill(ptr, HEAP_POISON, class_sizes[slab->class_index]);
    slab->u.free_map[slot / 64] |= bit;
    slab->free_count++;
    heap->stats.live_allocations--;
    heap->stats.live_bytes -= class_sizes[slab->class_index];

    if (slab->free_count == 1) {
        list_push(heap, slab); /* was full */
    }
    /* Give an empty slab back unless it is the only one with free slots:
     * keeping one per class avoids taking and returning a page on every
     * alloc/free pair. */
    if (slab->free_count == slab->capacity &&
        (heap->partial[slab->class_index] != slab || slab->next != NULL)) {
        list_remove(heap, slab);
        slab->magic = DEAD_MAGIC;
        heap->stats.slab_pages--;
        heap->ops->free_pages(heap->ops->ctx, slab, 1);
    }
    return HEAP_OK;
}

enum heap_status heap_free(struct heap *heap, void *ptr)
{
    struct heap_slab *h;

    if (ptr == NULL) {
        return HEAP_OK;
    }
    if (((uintptr_t)ptr & (HEAP_ALIGNMENT - 1)) != 0) {
        return HEAP_ERR_INVALID_POINTER;
    }
    h = header_of(ptr);
    if (h->magic == SLAB_MAGIC) {
        return free_small(heap, h, ptr);
    }
    if (h->magic == LARGE_MAGIC && (unsigned char *)ptr == (unsigned char *)h + HEADER_SIZE) {
        const size_t pages = h->pages;

        heap->stats.live_allocations--;
        heap->stats.live_bytes -= h->u.requested;
        heap->stats.large_pages -= pages;
        h->magic = DEAD_MAGIC; /* a second free is reported (until the page is reused) */
        heap->ops->free_pages(heap->ops->ctx, h, pages);
        return HEAP_OK;
    }
    return h->magic == DEAD_MAGIC ? HEAP_ERR_DOUBLE_FREE : HEAP_ERR_INVALID_POINTER;
}

size_t heap_block_size(const struct heap *heap, const void *ptr)
{
    const struct heap_slab *h;
    long slot;

    (void)heap;
    if (ptr == NULL || ((uintptr_t)ptr & (HEAP_ALIGNMENT - 1)) != 0) {
        return 0;
    }
    h = header_of(ptr);
    if (h->magic == LARGE_MAGIC &&
        (const unsigned char *)ptr == (const unsigned char *)h + HEADER_SIZE) {
        return (size_t)h->pages * HEAP_PAGE_SIZE - HEADER_SIZE;
    }
    if (h->magic != SLAB_MAGIC) {
        return 0;
    }
    slot = slot_of(h, ptr);
    if (slot < 0 || (h->u.free_map[slot / 64] & (1ull << ((unsigned long)slot % 64))) != 0) {
        return 0;
    }
    return class_sizes[h->class_index];
}

struct heap_stats heap_get_stats(const struct heap *heap)
{
    return heap->stats;
}

const char *heap_status_name(enum heap_status status)
{
    switch (status) {
    case HEAP_OK:
        return "ok";
    case HEAP_ERR_NO_MEMORY:
        return "out of memory";
    case HEAP_ERR_BAD_ARGUMENT:
        return "bad size";
    case HEAP_ERR_INVALID_POINTER:
        return "invalid pointer";
    case HEAP_ERR_DOUBLE_FREE:
        return "double free";
    }
    return "unknown";
}
