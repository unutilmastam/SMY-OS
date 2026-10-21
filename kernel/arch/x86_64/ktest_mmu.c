/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

/* ktests for the kernel's own page tables (Phase 5, docs/kernel/memory.md). */

#include "arch/mmu.h"
#include "arch/x86_64/descriptors.h"
#include "arch/x86_64/exceptions.h"
#include "arch/x86_64/gdt.h"
#include "arch/x86_64/regs.h"
#include "core/ktest.h"
#include "memory/phys.h"
#include "memory/vmm.h"

/* Defined in ktest_triggers.S. */
uint64_t ktest_trigger_load(uint64_t address);
void ktest_trigger_store(uint64_t address, uint64_t value);
void ktest_trigger_call(uint64_t address);
extern const char ktest_trigger_load_resume[];
extern const char ktest_trigger_store_resume[];
extern const char ktest_trigger_call_resume[];

/* Linker-script section bounds. */
extern const char kernel_start[];
extern const char kernel_end[];

#define PAGE ((uint64_t)ARCH_PAGE_SIZE)
#define PF_PRESENT 0x1u
#define PF_WRITE 0x2u
#define PF_FETCH 0x10u

/* One scratch page per test, so a test that fails half-way cannot break
 * the next one. */
enum {
    SCRATCH_MAP_UNMAP = 0,
    SCRATCH_READ_ONLY = 1,
    SCRATCH_NO_EXECUTE = 2,
};

static uint64_t scratch(unsigned slot)
{
    return VMM_SCRATCH_BASE + (uint64_t)slot * PAGE;
}

static const uint64_t rodata_object = 0x5359534F534D59ull; /* in .rodata */
static uint64_t data_object = 1;                           /* in .data */

static struct exception_expectation expect(uint64_t vector, const char *resume)
{
    return (struct exception_expectation){.vector = vector, .resume_rip = (uintptr_t)resume};
}

/* Map a fresh zeroed page at `virt`; returns its physical address. */
static uint64_t map_fresh_page(uint64_t virt, unsigned flags)
{
    uint64_t phys = 0;

    KTEST_ASSERT_EQ_U64(phys_alloc_page(&phys), PMM_OK);
    for (uint64_t i = 0; i < PAGE / 8; i++) {
        ((volatile uint64_t *)phys_to_virt(phys))[i] = 0;
    }
    KTEST_ASSERT_EQ_U64(arch_mmu_map(virt, phys, PAGE, flags), VM_OK);
    return phys;
}

static void unmap_and_free(uint64_t virt)
{
    uint64_t phys = 0;

    KTEST_ASSERT_EQ_U64(arch_mmu_unmap(virt, &phys), VM_OK);
    KTEST_ASSERT_EQ_U64(phys_free_page(phys), PMM_OK);
}

KTEST(mmu_cpu_protections_match_cpuid)
{
    const struct arch_mmu_features f = vmm_features();
    const struct cpuid_regs leaf7 = cpuid(7, 0);
    const bool smep = (leaf7.ebx & (1u << 7)) != 0;
    const bool smap = (leaf7.ebx & (1u << 20)) != 0;

    KTEST_ASSERT((read_msr(MSR_IA32_EFER) & EFER_NXE) != 0);
    KTEST_ASSERT((read_cr0() & CR0_WP) != 0);
    KTEST_ASSERT_EQ_U64((read_cr4() & CR4_SMEP) != 0, smep);
    KTEST_ASSERT_EQ_U64((read_cr4() & CR4_SMAP) != 0, smap);
    KTEST_ASSERT(f.no_execute && f.write_protect);
    KTEST_ASSERT_EQ_U64(f.smep, smep);
    KTEST_ASSERT_EQ_U64(f.smap, smap);
    if (f.write_combining) {
        KTEST_ASSERT_EQ_U64(read_msr(MSR_IA32_PAT), 0x0007010600070106ull);
    }
}

KTEST(mmu_kernel_sections_have_exact_rights)
{
    struct vm_translation t;
    uint64_t phys = 0;

    KTEST_ASSERT_EQ_U64(arch_mmu_translate((uintptr_t)&ktest_trigger_load, &t), VM_OK);
    KTEST_ASSERT_EQ_U64(t.flags, VM_EXEC);
    KTEST_ASSERT_EQ_U64(arch_mmu_translate((uintptr_t)&rodata_object, &t), VM_OK);
    KTEST_ASSERT_EQ_U64(t.flags, VM_READ);
    KTEST_ASSERT_EQ_U64(arch_mmu_translate((uintptr_t)&data_object, &t), VM_OK);
    KTEST_ASSERT_EQ_U64(t.flags, VM_WRITE);

    /* Direct map: writable, never executable, translates back to itself. */
    KTEST_ASSERT_EQ_U64(phys_alloc_page(&phys), PMM_OK);
    KTEST_ASSERT_EQ_U64(arch_mmu_translate((uintptr_t)phys_to_virt(phys), &t), VM_OK);
    KTEST_ASSERT_EQ_U64(t.flags, VM_WRITE);
    KTEST_ASSERT_EQ_U64(t.phys, phys);
    KTEST_ASSERT_EQ_U64(phys_free_page(phys), PMM_OK);
}

KTEST(mmu_no_kernel_page_is_writable_and_executable)
{
    unsigned mapped = 0;

    for (uintptr_t va = (uintptr_t)kernel_start; va < (uintptr_t)kernel_end; va += PAGE) {
        struct vm_translation t;

        if (arch_mmu_translate(va, &t) != VM_OK) {
            continue;
        }
        mapped++;
        if ((t.flags & VM_WRITE) != 0 && (t.flags & VM_EXEC) != 0) {
            ktest_fail(__FILE__, __LINE__, "page %p is writable and executable", (void *)va);
            return;
        }
    }
    KTEST_ASSERT(mapped > 0);
}

KTEST(mmu_map_write_read_unmap_then_fault)
{
    const uint64_t va = scratch(SCRATCH_MAP_UNMAP);
    struct exception_expectation e = expect(EXC_PAGE_FAULT, ktest_trigger_load_resume);
    const uint64_t phys = map_fresh_page(va, VM_WRITE);

    *(volatile uint64_t *)(va + 8) = 0x1234567890ABCDEFull;
    /* Same physical page through the direct map. */
    KTEST_ASSERT_EQ_U64(((volatile uint64_t *)phys_to_virt(phys))[1], 0x1234567890ABCDEFull);
    unmap_and_free(va);

    exceptions_expect(&e);
    (void)ktest_trigger_load(va + 8);
    KTEST_ASSERT(e.hit);
    KTEST_ASSERT_EQ_U64(e.cr2, va + 8);
    KTEST_ASSERT_EQ_U64(e.error_code & (PF_PRESENT | PF_WRITE), 0); /* not present, read */
}

KTEST(mmu_write_to_read_only_page_faults)
{
    const uint64_t va = scratch(SCRATCH_READ_ONLY);
    struct exception_expectation e = expect(EXC_PAGE_FAULT, ktest_trigger_store_resume);

    (void)map_fresh_page(va, VM_READ);
    KTEST_ASSERT_EQ_U64(ktest_trigger_load(va), 0); /* reading is fine */
    exceptions_expect(&e);
    ktest_trigger_store(va, 42);
    KTEST_ASSERT(e.hit);
    KTEST_ASSERT_EQ_U64(e.cr2, va);
    KTEST_ASSERT_EQ_U64(e.error_code & (PF_PRESENT | PF_WRITE), PF_PRESENT | PF_WRITE);
    KTEST_ASSERT_EQ_U64(ktest_trigger_load(va), 0); /* the write did not happen */

    /* After protect(VM_WRITE) the same store works. */
    KTEST_ASSERT_EQ_U64(arch_mmu_protect(va, VM_WRITE), VM_OK);
    ktest_trigger_store(va, 42);
    KTEST_ASSERT_EQ_U64(ktest_trigger_load(va), 42);
    unmap_and_free(va);
}

KTEST(mmu_write_to_kernel_rodata_faults)
{
    struct exception_expectation e = expect(EXC_PAGE_FAULT, ktest_trigger_store_resume);

    exceptions_expect(&e);
    ktest_trigger_store((uintptr_t)&rodata_object, 0);
    KTEST_ASSERT(e.hit);
    KTEST_ASSERT_EQ_U64(e.cr2, (uintptr_t)&rodata_object);
    KTEST_ASSERT_EQ_U64(e.error_code & (PF_PRESENT | PF_WRITE), PF_PRESENT | PF_WRITE);
    KTEST_ASSERT_EQ_U64(rodata_object, 0x5359534F534D59ull);
}

KTEST(mmu_execute_from_no_execute_page_faults)
{
    const uint64_t va = scratch(SCRATCH_NO_EXECUTE);
    struct exception_expectation e = expect(EXC_PAGE_FAULT, ktest_trigger_call_resume);

    (void)map_fresh_page(va, VM_WRITE);
    *(volatile uint8_t *)va = 0xC3; /* ret */
    exceptions_expect(&e);
    ktest_trigger_call(va);
    KTEST_ASSERT(e.hit);
    KTEST_ASSERT_EQ_U64(e.fault_rip, va);
    KTEST_ASSERT_EQ_U64(e.cr2, va);
    KTEST_ASSERT_EQ_U64(e.error_code & (PF_PRESENT | PF_FETCH), PF_PRESENT | PF_FETCH);
    unmap_and_free(va);
}

KTEST(mmu_kernel_stacks_have_guard_pages)
{
    struct exception_expectation e = expect(EXC_PAGE_FAULT, ktest_trigger_load_resume);
    struct vm_translation t;
    const uintptr_t sp = (uintptr_t)__builtin_frame_address(0);
    uintptr_t lo = 0;
    uintptr_t hi = 0;

    KTEST_ASSERT(vmm_stack_bounds(sp, &lo, &hi)); /* running on a guarded stack */
    KTEST_ASSERT_EQ_U64(arch_mmu_translate(lo - PAGE, &t), VM_ERR_NOT_MAPPED);
    KTEST_ASSERT(vmm_stack_guard_hit(lo - 8, &lo, &hi));
    exceptions_expect(&e);
    (void)ktest_trigger_load(lo - 8);
    KTEST_ASSERT(e.hit);
    KTEST_ASSERT_EQ_U64(e.cr2, lo - 8);

    /* The exception stacks (IST) are guarded too. */
    KTEST_ASSERT(vmm_stack_bounds(gdt_ist_top(IST_DOUBLE_FAULT) - 8, &lo, &hi));
    KTEST_ASSERT(vmm_stack_bounds(gdt_ist_top(IST_NMI) - 8, &lo, &hi));
    KTEST_ASSERT(vmm_stack_bounds(gdt_ist_top(IST_MACHINE_CHECK) - 8, &lo, &hi));
}

/* Deep recursion: each level keeps 512 bytes live so the compiler cannot
 * turn it into a loop; the limit (volatile, so not provable) is far beyond
 * any kernel stack. Must end in a double fault on the IST stack with a
 * stack-overflow report (tests/integration/integration.mk). */
static volatile unsigned recursion_limit = 1u << 30;

static unsigned recurse(unsigned depth) // NOLINT(misc-no-recursion): overflow on purpose
{
    volatile uint8_t buffer[512];

    if (depth >= recursion_limit) {
        return 0;
    }
    buffer[0] = (uint8_t)depth;
    return recurse(depth + 1) + buffer[0];
}

KTEST_PANIC(panic_on_kernel_stack_overflow)
{
    (void)recurse(0);
}
