/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

#include "memory/vmm.h"

#include "core/log.h"
#include "core/panic.h"
#include "memory/phys.h"

/* Page-aligned section bounds from the linker script. */
extern const char kernel_start[];
extern const char kernel_text_start[];
extern const char kernel_text_end[];
extern const char kernel_rodata_start[];
extern const char kernel_rodata_end[];
extern const char kernel_data_start[];
extern const char kernel_end[];

#define PAGE ((uint64_t)ARCH_PAGE_SIZE)
/* Stack slots are this far apart: room for the largest stack plus guard. */
#define STACK_SLOT_SIZE 0x100000ull /* 1 MiB */

struct stack_record {
    uintptr_t lo; /* lowest mapped byte */
    uintptr_t hi; /* one past the top */
};

static struct stack_record stacks[VMM_MAX_STACKS];
static size_t stack_count;
static struct arch_mmu_features features;

static uint64_t align_down(uint64_t v)
{
    return v & ~(PAGE - 1);
}

static uint64_t align_up(uint64_t v)
{
    return (v + PAGE - 1) & ~(PAGE - 1);
}

static void map_or_panic(uint64_t virt, uint64_t phys, uint64_t size, unsigned flags,
                         const char *what)
{
    const enum vm_status st = arch_mmu_map(virt, phys, size, flags);

    if (st != VM_OK) {
        panic("vmm: mapping %s at %p failed: %s", what, (void *)virt, vm_status_name(st));
    }
}

static void map_direct(const struct limine_memmap_response *memmap, uint64_t hhdm,
                       uint64_t *mapped_bytes)
{
    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        const struct limine_memmap_entry *e = memmap->entries[i];
        const uint64_t start = align_down(e->base);
        const uint64_t end = align_up(e->base + e->length);
        unsigned flags = VM_WRITE;

        if (e->type == LIMINE_MEMMAP_RESERVED || e->type == LIMINE_MEMMAP_BAD_MEMORY ||
            end <= start) {
            continue;
        }
        if (e->type == LIMINE_MEMMAP_FRAMEBUFFER) {
            flags |= VM_WRITE_COMBINING;
        }
        /*
         * Entries can share a page at unaligned edges; map only what is not
         * mapped yet, page by page at the edges, so overlaps are harmless.
         */
        for (uint64_t p = start; p < end;) {
            struct vm_translation t;

            if (arch_mmu_translate(hhdm + p, &t) == VM_OK) {
                p += PAGE;
                continue;
            }
            /* Largest run starting at p that is still unmapped: map pages
             * one by one at unaligned edges, the aligned middle in bulk. */
            if ((p & 0x1FFFFF) == 0 && end - p >= 0x200000) {
                const uint64_t bulk = (end - p) & ~0x1FFFFFull;

                map_or_panic(hhdm + p, p, bulk, flags, "direct map");
                *mapped_bytes += bulk;
                p += bulk;
            } else {
                map_or_panic(hhdm + p, p, PAGE, flags, "direct map");
                *mapped_bytes += PAGE;
                p += PAGE;
            }
        }
    }
}

static void map_kernel_image(const struct vmm_kernel_image *image)
{
    const struct {
        const char *start;
        const char *end;
        unsigned flags;
        const char *name;
    } sections[] = {
        {kernel_start, kernel_text_start, VM_WRITE, "kernel requests"},
        {kernel_text_start, kernel_text_end, VM_EXEC, "kernel text"},
        {kernel_rodata_start, kernel_rodata_end, VM_READ, "kernel rodata"},
        {kernel_data_start, kernel_end, VM_WRITE, "kernel data"},
    };

    if ((uintptr_t)kernel_start != image->virt_base) {
        panic("vmm: kernel linked at %p but loaded at %p", (const void *)kernel_start,
              (void *)image->virt_base);
    }
    for (size_t i = 0; i < sizeof sections / sizeof sections[0]; i++) {
        const uint64_t virt = align_down((uintptr_t)sections[i].start);
        const uint64_t size = align_up((uintptr_t)sections[i].end) - virt;

        map_or_panic(virt, virt - image->virt_base + image->phys_base, size, sections[i].flags,
                     sections[i].name);
    }
}

void vmm_init(const struct limine_memmap_response *memmap, uint64_t hhdm_offset,
              const struct vmm_kernel_image *image)
{
    uint64_t direct_bytes = 0;
    const struct pmm_stats before = phys_stats();
    uint64_t table_kib;
    enum vm_status st;

    if (memmap == NULL || image == NULL) {
        panic("vmm: missing memory map or kernel address");
        return; /* not reached; for cppcheck */
    }
    st = arch_mmu_init();
    if (st != VM_OK) {
        panic("vmm: cannot create the kernel address space: %s", vm_status_name(st));
    }
    map_direct(memmap, hhdm_offset, &direct_bytes);
    map_kernel_image(image);
    features = arch_mmu_activate();
    table_kib = (before.free_frames - phys_stats().free_frames) * (PAGE / 1024);

    klog(LOG_INFO,
         "vmm: own page tables active; direct map %llu MiB, tables %llu KiB; NX %s, WP %s, "
         "SMEP %s, SMAP %s, WC %s",
         (unsigned long long)(direct_bytes >> 20), (unsigned long long)table_kib,
         features.no_execute ? "on" : "off", features.write_protect ? "on" : "off",
         features.smep ? "on" : "off", features.smap ? "on" : "off",
         features.write_combining ? "on" : "off");
}

struct arch_mmu_features vmm_features(void)
{
    return features;
}

uintptr_t vmm_alloc_stack(size_t pages)
{
    uint64_t base;
    uint64_t top;

    if (pages == 0 || (pages + 1) * PAGE > STACK_SLOT_SIZE) {
        panic("vmm: bad kernel stack size %zu pages", pages);
    }
    if (stack_count == VMM_MAX_STACKS) {
        panic("vmm: out of kernel stack slots (%d)", VMM_MAX_STACKS);
    }
    /* Slot layout: [guard page][stack pages] ... unused up to the slot end. */
    base = VMM_STACK_BASE + stack_count * STACK_SLOT_SIZE + PAGE;
    top = base + pages * PAGE;
    for (uint64_t va = base; va < top; va += PAGE) {
        uint64_t phys;

        if (phys_alloc_page(&phys) != PMM_OK) {
            panic("vmm: out of memory for a kernel stack");
        }
        map_or_panic(va, phys, PAGE, VM_WRITE, "kernel stack");
    }
    stacks[stack_count].lo = (uintptr_t)base;
    stacks[stack_count].hi = (uintptr_t)top;
    stack_count++;
    return (uintptr_t)top;
}

bool vmm_stack_bounds(uintptr_t addr, uintptr_t *lo, uintptr_t *hi)
{
    for (size_t i = 0; i < stack_count; i++) {
        if (addr >= stacks[i].lo && addr < stacks[i].hi) {
            *lo = stacks[i].lo;
            *hi = stacks[i].hi;
            return true;
        }
    }
    return false;
}

bool vmm_stack_guard_hit(uintptr_t addr, uintptr_t *lo, uintptr_t *hi)
{
    for (size_t i = 0; i < stack_count; i++) {
        if (addr >= stacks[i].lo - PAGE && addr < stacks[i].lo) {
            *lo = stacks[i].lo;
            *hi = stacks[i].hi;
            return true;
        }
    }
    return false;
}

void vmm_describe_fault(uintptr_t addr)
{
    uintptr_t lo;
    uintptr_t hi;

    if (vmm_stack_guard_hit(addr, &lo, &hi)) {
        log_printf("SMY-OS:   kernel stack overflow: guard page below the stack %p-%p was hit\n",
                   (void *)lo, (void *)hi);
    }
    arch_mmu_describe(addr);
}
