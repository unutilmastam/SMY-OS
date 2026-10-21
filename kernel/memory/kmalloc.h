/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

/*
 * Kernel heap: kmalloc/kfree on top of the heap core (memory/heap.h) with
 * pages from the physical memory manager, reached through the direct map
 * (docs/kernel/memory.md). Blocks are 16-byte aligned and not zeroed
 * (kzalloc zeroes). Large blocks (> 1 KiB) need physically contiguous pages.
 */

#ifndef SMY_KERNEL_MEMORY_KMALLOC_H
#define SMY_KERNEL_MEMORY_KMALLOC_H

#include "memory/heap.h"

#include <stddef.h>

void kmalloc_init(void);

/* NULL when out of memory or for size 0. */
void *kmalloc(size_t size);
void *kzalloc(size_t size);

/* NULL is ignored. An invalid pointer or a double free panics. */
void kfree(void *ptr);

struct heap_stats kmalloc_stats(void);

#endif /* SMY_KERNEL_MEMORY_KMALLOC_H */
