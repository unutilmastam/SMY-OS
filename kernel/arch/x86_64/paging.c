/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

#include "arch/x86_64/paging.h"

#include <stddef.h>

#define LARGE_MASK ((uint64_t)PAGING_LARGE_PAGE_SIZE - 1)
#define PAGE_MASK ((uint64_t)PAGING_PAGE_SIZE - 1)
/* Physical address bits of a 2 MiB PDE (bit 12 is the PAT bit there). */
#define LARGE_ADDRESS_MASK (PTE_ADDRESS_MASK & ~LARGE_MASK)

/* Index into the table at `level` (3 = PML4 ... 0 = PT). */
static unsigned table_index(uint64_t virt, unsigned level)
{
    return (unsigned)((virt >> (12u + 9u * level)) & (PAGING_ENTRIES - 1u));
}

static uint64_t *table(const struct address_space *as, uint64_t phys)
{
    return as->ops->table_ptr(as->ops->ctx, phys);
}

bool paging_is_canonical(uint64_t virt)
{
    const uint64_t top = virt >> 47;

    return top == 0 || top == 0x1FFFF;
}

static bool flags_valid(unsigned flags)
{
    const unsigned known = VM_WRITE | VM_EXEC | VM_USER | VM_WRITE_COMBINING | VM_UNCACHED;

    return (flags & ~known) == 0 &&
           (flags & (VM_WRITE_COMBINING | VM_UNCACHED)) != (VM_WRITE_COMBINING | VM_UNCACHED);
}

/* Leaf entry bits for `flags` (the same positions in a PTE and a 2 MiB PDE). */
static uint64_t leaf_bits(unsigned flags)
{
    uint64_t bits = PTE_PRESENT;

    if ((flags & VM_WRITE) != 0) {
        bits |= PTE_WRITABLE;
    }
    if ((flags & VM_USER) != 0) {
        bits |= PTE_USER;
    }
    if ((flags & VM_EXEC) == 0) {
        bits |= PTE_NO_EXECUTE;
    }
    if ((flags & VM_WRITE_COMBINING) != 0) {
        bits |= PTE_PWT;
    }
    if ((flags & VM_UNCACHED) != 0) {
        bits |= PTE_PCD;
    }
    return bits;
}

static unsigned leaf_flags(uint64_t entry)
{
    unsigned flags = VM_READ;

    if ((entry & PTE_WRITABLE) != 0) {
        flags |= VM_WRITE;
    }
    if ((entry & PTE_USER) != 0) {
        flags |= VM_USER;
    }
    if ((entry & PTE_NO_EXECUTE) == 0) {
        flags |= VM_EXEC;
    }
    if ((entry & PTE_PWT) != 0) {
        flags |= VM_WRITE_COMBINING;
    }
    if ((entry & PTE_PCD) != 0) {
        flags |= VM_UNCACHED;
    }
    return flags;
}

static enum vm_status new_table(struct address_space *as, uint64_t *phys_out)
{
    uint64_t phys;
    uint64_t *t;

    if (!as->ops->alloc_table(as->ops->ctx, &phys)) {
        return VM_ERR_NO_MEMORY;
    }
    t = table(as, phys);
    for (unsigned i = 0; i < PAGING_ENTRIES; i++) {
        t[i] = 0;
    }
    as->table_pages++;
    *phys_out = phys;
    return VM_OK;
}

enum vm_status paging_init(struct address_space *as, const struct paging_ops *ops)
{
    as->ops = ops;
    as->table_pages = 0;
    return new_table(as, &as->root);
}

/*
 * Walk from the root down to the table at `target_level`, creating missing
 * tables if `create`. Returns that table, or NULL with *status set.
 * Intermediate entries are permissive (present, writable, user if the leaf
 * is user); the leaf entry decides the real rights.
 */
static uint64_t *walk_to(struct address_space *as, uint64_t virt, unsigned target_level,
                         bool create, bool user, enum vm_status *status)
{
    uint64_t *t = table(as, as->root);

    for (unsigned level = 3; level > target_level; level--) {
        uint64_t *entry = &t[table_index(virt, level)];

        if ((*entry & PTE_PRESENT) == 0) {
            uint64_t phys;

            if (!create) {
                *status = VM_ERR_NOT_MAPPED;
                return NULL;
            }
            *status = new_table(as, &phys);
            if (*status != VM_OK) {
                return NULL;
            }
            *entry = phys | PTE_PRESENT | PTE_WRITABLE;
        } else if ((*entry & PTE_LARGE) != 0) {
            *status = VM_ERR_LARGE_PAGE;
            return NULL;
        }
        if (user) {
            *entry |= PTE_USER;
        }
        t = table(as, *entry & PTE_ADDRESS_MASK);
    }
    *status = VM_OK;
    return t;
}

static void invalidate(const struct address_space *as, uint64_t virt)
{
    if (as->ops->invalidate != NULL) {
        as->ops->invalidate(as->ops->ctx, virt);
    }
}

enum vm_status paging_map(struct address_space *as, uint64_t virt, uint64_t phys, unsigned flags)
{
    enum vm_status st;
    uint64_t *pt;
    uint64_t *entry;

    if ((virt & PAGE_MASK) != 0 || (phys & PAGE_MASK) != 0 || (phys & ~PTE_ADDRESS_MASK) != 0 ||
        !paging_is_canonical(virt) || !flags_valid(flags)) {
        return VM_ERR_BAD_ARGUMENT;
    }
    pt = walk_to(as, virt, 0, true, (flags & VM_USER) != 0, &st);
    if (pt == NULL) {
        return st;
    }
    entry = &pt[table_index(virt, 0)];
    if ((*entry & PTE_PRESENT) != 0) {
        return VM_ERR_ALREADY_MAPPED;
    }
    *entry = phys | leaf_bits(flags);
    return VM_OK;
}

enum vm_status paging_map_large(struct address_space *as, uint64_t virt, uint64_t phys,
                                unsigned flags)
{
    enum vm_status st;
    uint64_t *pd;
    uint64_t *entry;

    if ((virt & LARGE_MASK) != 0 || (phys & LARGE_MASK) != 0 || (phys & ~PTE_ADDRESS_MASK) != 0 ||
        !paging_is_canonical(virt) || !flags_valid(flags)) {
        return VM_ERR_BAD_ARGUMENT;
    }
    pd = walk_to(as, virt, 1, true, (flags & VM_USER) != 0, &st);
    if (pd == NULL) {
        return st;
    }
    entry = &pd[table_index(virt, 1)];
    if ((*entry & PTE_PRESENT) != 0) {
        return VM_ERR_ALREADY_MAPPED;
    }
    *entry = phys | leaf_bits(flags) | PTE_LARGE;
    return VM_OK;
}

enum vm_status paging_map_range(struct address_space *as, uint64_t virt, uint64_t phys,
                                uint64_t size, unsigned flags)
{
    if ((virt & PAGE_MASK) != 0 || (phys & PAGE_MASK) != 0 || (size & PAGE_MASK) != 0 ||
        virt + size < virt || phys + size < phys) {
        return VM_ERR_BAD_ARGUMENT;
    }
    while (size > 0) {
        enum vm_status st;
        uint64_t step;

        if ((virt & LARGE_MASK) == 0 && (phys & LARGE_MASK) == 0 &&
            size >= PAGING_LARGE_PAGE_SIZE) {
            step = PAGING_LARGE_PAGE_SIZE;
            st = paging_map_large(as, virt, phys, flags);
        } else {
            step = PAGING_PAGE_SIZE;
            st = paging_map(as, virt, phys, flags);
        }
        if (st != VM_OK) {
            return st;
        }
        virt += step;
        phys += step;
        size -= step;
    }
    return VM_OK;
}

/* The present 4 KiB leaf entry of `virt`, or NULL with *status set. */
static uint64_t *leaf_entry(struct address_space *as, uint64_t virt, enum vm_status *status)
{
    uint64_t *pt;
    uint64_t *entry;

    if ((virt & PAGE_MASK) != 0 || !paging_is_canonical(virt)) {
        *status = VM_ERR_BAD_ARGUMENT;
        return NULL;
    }
    pt = walk_to(as, virt, 0, false, false, status);
    if (pt == NULL) {
        return NULL;
    }
    entry = &pt[table_index(virt, 0)];
    if ((*entry & PTE_PRESENT) == 0) {
        *status = VM_ERR_NOT_MAPPED;
        return NULL;
    }
    *status = VM_OK;
    return entry;
}

enum vm_status paging_unmap(struct address_space *as, uint64_t virt, uint64_t *phys_out)
{
    enum vm_status st;
    uint64_t *entry = leaf_entry(as, virt, &st);

    if (entry == NULL) {
        return st;
    }
    if (phys_out != NULL) {
        *phys_out = *entry & PTE_ADDRESS_MASK;
    }
    *entry = 0;
    invalidate(as, virt);
    return VM_OK;
}

enum vm_status paging_protect(struct address_space *as, uint64_t virt, unsigned flags)
{
    enum vm_status st;
    uint64_t *entry;

    if (!flags_valid(flags)) {
        return VM_ERR_BAD_ARGUMENT;
    }
    entry = leaf_entry(as, virt, &st);
    if (entry == NULL) {
        return st;
    }
    *entry = (*entry & PTE_ADDRESS_MASK) | leaf_bits(flags);
    invalidate(as, virt);
    return VM_OK;
}

void paging_walk(const struct address_space *as, uint64_t virt, struct vm_walk *out)
{
    const uint64_t *t = table(as, as->root);

    out->levels = 0;
    for (unsigned level = 3;; level--) {
        const uint64_t entry = t[table_index(virt, level)];

        out->entries[out->levels++] = entry;
        if ((entry & PTE_PRESENT) == 0 || level == 0 || (level == 1 && (entry & PTE_LARGE) != 0)) {
            return;
        }
        t = table(as, entry & PTE_ADDRESS_MASK);
    }
}

enum vm_status paging_translate(const struct address_space *as, uint64_t virt,
                                struct vm_translation *out)
{
    struct vm_walk w;
    uint64_t leaf;

    if (!paging_is_canonical(virt)) {
        return VM_ERR_BAD_ARGUMENT;
    }
    paging_walk(as, virt, &w);
    leaf = w.entries[w.levels - 1];
    if ((leaf & PTE_PRESENT) == 0 || w.levels < 3) {
        return VM_ERR_NOT_MAPPED; /* 1 GiB pages are never created */
    }
    if (w.levels == 3) {
        out->page_size = PAGING_LARGE_PAGE_SIZE;
        out->phys = (leaf & LARGE_ADDRESS_MASK) | (virt & LARGE_MASK);
    } else {
        out->page_size = PAGING_PAGE_SIZE;
        out->phys = (leaf & PTE_ADDRESS_MASK) | (virt & PAGE_MASK);
    }
    out->flags = leaf_flags(leaf);
    return VM_OK;
}

const char *vm_status_name(enum vm_status status)
{
    switch (status) {
    case VM_OK:
        return "ok";
    case VM_ERR_NO_MEMORY:
        return "out of memory";
    case VM_ERR_BAD_ARGUMENT:
        return "bad argument";
    case VM_ERR_ALREADY_MAPPED:
        return "already mapped";
    case VM_ERR_NOT_MAPPED:
        return "not mapped";
    case VM_ERR_LARGE_PAGE:
        return "inside a 2 MiB page";
    }
    return "unknown";
}
