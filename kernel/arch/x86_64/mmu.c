/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

/*
 * x86-64 side of arch/mmu.h: the kernel's own page tables (ADR 0012).
 * Table pages come from the physical memory manager and are reached through
 * the direct map, which stays valid across the switch because the new
 * tables map it at the same offset as the bootloader's.
 */

#include "arch/mmu.h"

#include "arch/x86_64/paging.h"
#include "arch/x86_64/regs.h"
#include "core/log.h"
#include "core/panic.h"
#include "memory/phys.h"

#include <stddef.h>

/*
 * PAT layout (Intel SDM Vol. 3A, 13.12): the power-on default with entry 1
 * changed from write-through to write-combining, so PWT=1 (PCD=0, PAT=0)
 * selects WC. Entries 0..7: WB, WC, UC-, UC, WB, WC, UC-, UC.
 */
#define PAT_VALUE 0x0007010600070106ull

static bool alloc_table(void *ctx, uint64_t *phys_out)
{
    (void)ctx;
    return phys_alloc_page(phys_out) == PMM_OK;
}

static void *table_ptr(void *ctx, uint64_t phys)
{
    (void)ctx;
    return phys_to_virt(phys);
}

static bool active;

static void invalidate_page(void *ctx, uint64_t virt)
{
    (void)ctx;
    if (active) {
        invlpg(virt);
    }
}

static const struct paging_ops kernel_ops = {
    .alloc_table = alloc_table,
    .table_ptr = table_ptr,
    .invalidate = invalidate_page,
    .ctx = NULL,
};

static struct address_space kernel_space;

enum vm_status arch_mmu_init(void)
{
    return paging_init(&kernel_space, &kernel_ops);
}

enum vm_status arch_mmu_map(uint64_t virt, uint64_t phys, uint64_t size, unsigned flags)
{
    return paging_map_range(&kernel_space, virt, phys, size, flags);
}

enum vm_status arch_mmu_unmap(uint64_t virt, uint64_t *phys_out)
{
    return paging_unmap(&kernel_space, virt, phys_out);
}

enum vm_status arch_mmu_protect(uint64_t virt, unsigned flags)
{
    return paging_protect(&kernel_space, virt, flags);
}

enum vm_status arch_mmu_translate(uint64_t virt, struct vm_translation *out)
{
    return paging_translate(&kernel_space, virt, out);
}

struct arch_mmu_features arch_mmu_activate(void)
{
    struct arch_mmu_features f = {0};
    const struct cpuid_regs ext = cpuid(0x80000001u, 0);
    const struct cpuid_regs basic = cpuid(1, 0);
    const struct cpuid_regs leaf7 = cpuid(0, 0).eax >= 7 ? cpuid(7, 0) : (struct cpuid_regs){0};
    uint64_t cr4;

    if ((ext.edx & (1u << 20)) == 0) {
        panic("CPU has no no-execute (NX) support; SMY-OS requires it");
    }
    write_msr(MSR_IA32_EFER, read_msr(MSR_IA32_EFER) | EFER_NXE);
    f.no_execute = true;

    if ((basic.edx & (1u << 16)) != 0) {
        /* Caches must not hold lines with the old memory type. */
        wbinvd();
        write_msr(MSR_IA32_PAT, PAT_VALUE);
        f.write_combining = true;
    }

    write_cr3(kernel_space.root);
    active = true;
    /* Also drop global TLB entries the bootloader may have left behind. */
    cr4 = read_cr4();
    if ((cr4 & CR4_PGE) != 0) {
        write_cr4(cr4 & ~(uint64_t)CR4_PGE);
        write_cr4(cr4);
    }

    write_cr0(read_cr0() | CR0_WP);
    f.write_protect = true;

    cr4 = read_cr4();
    if ((leaf7.ebx & (1u << 7)) != 0) {
        cr4 |= CR4_SMEP;
        f.smep = true;
    }
    if ((leaf7.ebx & (1u << 20)) != 0) {
        cr4 |= CR4_SMAP;
        f.smap = true;
    }
    write_cr4(cr4);
    return f;
}

void arch_mmu_describe(uint64_t virt)
{
    static const char *const names[4] = {"PML4E", "PDPTE", "PDE", "PTE"};
    struct vm_walk w = {0};

    if (!active) {
        log_write("SMY-OS:   page walk: bootloader page tables active, not described\n");
        return;
    }
    if (!paging_is_canonical(virt)) {
        log_write("SMY-OS:   page walk: address is not canonical\n");
        return;
    }
    paging_walk(&kernel_space, virt, &w);
    if (w.levels == 0) {
        return; /* not reached: a walk reads at least the PML4 entry */
    }
    log_write("SMY-OS:   page walk:");
    for (unsigned i = 0; i < w.levels; i++) {
        log_printf(" %s=%016llx", names[i], (unsigned long long)w.entries[i]);
    }
    log_write("\n");
    if ((w.entries[w.levels - 1] & PTE_PRESENT) == 0) {
        log_printf("SMY-OS:   page walk: not present at %s\n", names[w.levels - 1]);
    } else {
        const uint64_t leaf = w.entries[w.levels - 1];

        log_printf("SMY-OS:   page walk: %s page, %s%s%s\n", w.levels == 3 ? "2 MiB" : "4 KiB",
                   (leaf & PTE_WRITABLE) != 0 ? "read-write" : "read-only",
                   (leaf & PTE_NO_EXECUTE) != 0 ? ", no-execute" : ", executable",
                   (leaf & PTE_USER) != 0 ? ", user" : ", kernel");
    }
}
