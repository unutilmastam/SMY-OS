/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

/* ktests for x86-64 descriptor tables and exception handling (Phase 3). */

#include "arch/x86_64/descriptors.h"
#include "arch/x86_64/exceptions.h"
#include "arch/x86_64/idt.h"
#include "arch/x86_64/regs.h"
#include "core/ktest.h"

/* Defined in ktest_triggers.S. */
void ktest_trigger_ud(void);
void ktest_trigger_bp(void);
void ktest_trigger_de(void);
uint64_t ktest_trigger_load(uint64_t address);
extern const char ktest_trigger_ud_resume[];
extern const char ktest_trigger_bp_resume[];
extern const char ktest_trigger_de_resume[];
extern const char ktest_trigger_load_resume[];

/* Inside the kernel's 2 GiB window but 256 MiB above the image, so nothing
 * is mapped there (Limine maps only the kernel's segments; the HHDM lives at
 * 0xffff8000...). Valid until the kernel image grows past 256 MiB. */
#define UNMAPPED_KERNEL_ADDRESS 0xffffffff90000000u
/* Bit 63 set, bits 62..47 not all equal: non-canonical -> #GP, not #PF. */
#define NON_CANONICAL_ADDRESS 0x8000000000000000u

static struct exception_expectation expect(uint64_t vector, const char *resume)
{
    return (struct exception_expectation){.vector = vector, .resume_rip = (uintptr_t)resume};
}

KTEST(cpu_gdt_and_tss_are_loaded)
{
    const struct descriptor_pointer gdtr = read_gdtr();

    KTEST_ASSERT_EQ_U64(gdtr.limit, GDT_ENTRIES * 8 - 1);
    KTEST_ASSERT_EQ_U64(read_task_register(), GDT_TSS);
}

KTEST(cpu_idt_has_exception_gates_with_ist)
{
    const struct descriptor_pointer idtr = read_idtr();

    KTEST_ASSERT_EQ_U64(idtr.limit, IDT_ENTRIES * sizeof(struct idt_gate) - 1);
    for (unsigned v = 0; v < 32; v++) {
        const struct idt_gate *g = idt_gate(v);
        KTEST_ASSERT_EQ_U64(g->type_attr, IDT_GATE_INTERRUPT_DPL0);
        KTEST_ASSERT_EQ_U64(g->selector, GDT_KERNEL_CODE);
        KTEST_ASSERT(idt_gate_handler(g) != 0);
    }
    KTEST_ASSERT_EQ_U64(idt_gate(EXC_DOUBLE_FAULT)->ist, IST_DOUBLE_FAULT);
    KTEST_ASSERT_EQ_U64(idt_gate(EXC_NMI)->ist, IST_NMI);
    KTEST_ASSERT_EQ_U64(idt_gate(EXC_MACHINE_CHECK)->ist, IST_MACHINE_CHECK);
    KTEST_ASSERT_EQ_U64(idt_gate(EXC_PAGE_FAULT)->ist, IST_NONE);
}

KTEST(cpu_catches_invalid_opcode)
{
    struct exception_expectation e = expect(EXC_INVALID_OPCODE, ktest_trigger_ud_resume);

    exceptions_expect(&e);
    ktest_trigger_ud();
    KTEST_ASSERT(e.hit);
    KTEST_ASSERT_EQ_U64(e.fault_rip, (uintptr_t)ktest_trigger_ud);
}

KTEST(cpu_catches_breakpoint)
{
    struct exception_expectation e = expect(EXC_BREAKPOINT, ktest_trigger_bp_resume);

    exceptions_expect(&e);
    ktest_trigger_bp();
    KTEST_ASSERT(e.hit);
    /* #BP is a trap: the saved RIP is already past the int3. */
    KTEST_ASSERT_EQ_U64(e.fault_rip, (uintptr_t)ktest_trigger_bp_resume);
}

KTEST(cpu_catches_page_fault_with_address)
{
    struct exception_expectation e = expect(EXC_PAGE_FAULT, ktest_trigger_load_resume);

    exceptions_expect(&e);
    (void)ktest_trigger_load(UNMAPPED_KERNEL_ADDRESS);
    KTEST_ASSERT(e.hit);
    KTEST_ASSERT_EQ_U64(e.cr2, UNMAPPED_KERNEL_ADDRESS);
    /* Error code: not present (P=0), read (W=0), supervisor (U=0). */
    KTEST_ASSERT_EQ_U64(e.error_code & 0x7u, 0);
    KTEST_ASSERT_EQ_U64(e.fault_rip, (uintptr_t)ktest_trigger_load);
}

KTEST(cpu_catches_general_protection)
{
    struct exception_expectation e = expect(EXC_GENERAL_PROTECTION, ktest_trigger_load_resume);

    exceptions_expect(&e);
    (void)ktest_trigger_load(NON_CANONICAL_ADDRESS);
    KTEST_ASSERT(e.hit);
    KTEST_ASSERT_EQ_U64(e.error_code, 0);
}

KTEST(cpu_catches_divide_error)
{
    struct exception_expectation e = expect(EXC_DIVIDE_ERROR, ktest_trigger_de_resume);

    exceptions_expect(&e);
    ktest_trigger_de();
    KTEST_ASSERT(e.hit);
}

KTEST(cpu_survives_repeated_exceptions)
{
    /* The handler must restore every register and the stack each time. */
    for (int i = 0; i < 1000; i++) {
        struct exception_expectation e = expect(EXC_INVALID_OPCODE, ktest_trigger_ud_resume);

        exceptions_expect(&e);
        ktest_trigger_ud();
        if (!e.hit) {
            KTEST_ASSERT(e.hit);
            return;
        }
    }
}

/* Must end in a panic with a register dump (checked by
 * tests/integration/ktest.sh); run only when selected by name. */
KTEST_PANIC(panic_on_unhandled_invalid_opcode)
{
    ktest_trigger_ud();
}

KTEST_PANIC(panic_on_unhandled_page_fault)
{
    (void)ktest_trigger_load(UNMAPPED_KERNEL_ADDRESS);
}
