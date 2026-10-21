/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

#include "arch/x86_64/exceptions.h"

#include "arch/x86_64/regs.h"
#include "core/log.h"
#include "core/panic.h"
#include "memory/vmm.h"

#include <stddef.h>

struct exception_info {
    const char *mnemonic;
    const char *description;
};

/* Intel SDM Vol. 3A, Table 6-1. */
static const struct exception_info exception_table[32] = {
    [0] = {"#DE", "divide error"},
    [1] = {"#DB", "debug"},
    [2] = {"NMI", "non-maskable interrupt"},
    [3] = {"#BP", "breakpoint"},
    [4] = {"#OF", "overflow"},
    [5] = {"#BR", "bound range exceeded"},
    [6] = {"#UD", "invalid opcode"},
    [7] = {"#NM", "device not available"},
    [8] = {"#DF", "double fault"},
    [9] = {"CSO", "coprocessor segment overrun"},
    [10] = {"#TS", "invalid TSS"},
    [11] = {"#NP", "segment not present"},
    [12] = {"#SS", "stack-segment fault"},
    [13] = {"#GP", "general protection"},
    [14] = {"#PF", "page fault"},
    [15] = {"--", "reserved"},
    [16] = {"#MF", "x87 floating-point error"},
    [17] = {"#AC", "alignment check"},
    [18] = {"#MC", "machine check"},
    [19] = {"#XM", "SIMD floating-point exception"},
    [20] = {"#VE", "virtualization exception"},
    [21] = {"#CP", "control protection"},
    [22] = {"--", "reserved"},
    [23] = {"--", "reserved"},
    [24] = {"--", "reserved"},
    [25] = {"--", "reserved"},
    [26] = {"--", "reserved"},
    [27] = {"--", "reserved"},
    [28] = {"#HV", "hypervisor injection"},
    [29] = {"#VC", "VMM communication"},
    [30] = {"#SX", "security exception"},
    [31] = {"--", "reserved"},
};

static struct exception_expectation *armed;

const char *exception_mnemonic(uint64_t vector)
{
    return vector < 32 ? exception_table[vector].mnemonic : "IRQ";
}

const char *exception_description(uint64_t vector)
{
    return vector < 32 ? exception_table[vector].description : "unexpected interrupt";
}

void exceptions_expect(struct exception_expectation *expectation)
{
    if (expectation != NULL) {
        expectation->hit = false;
    }
    armed = expectation;
}

static void dump_page_fault_details(uint64_t error_code, uint64_t cr2)
{
    log_printf("SMY-OS:   fault address (CR2) %p: %s, %s, %s mode%s%s\n", (void *)cr2,
               (error_code & 1u) != 0 ? "protection violation" : "page not present",
               (error_code & 2u) != 0 ? "write" : "read",
               (error_code & 4u) != 0 ? "user" : "kernel",
               (error_code & 8u) != 0 ? ", reserved bit set" : "",
               (error_code & 16u) != 0 ? ", instruction fetch" : "");
}

static void dump_registers(const struct interrupt_frame *f)
{
    log_printf("SMY-OS:   RIP=%016llx  CS=%04llx  RFLAGS=%016llx\n", (unsigned long long)f->rip,
               (unsigned long long)f->cs, (unsigned long long)f->rflags);
    log_printf("SMY-OS:   RSP=%016llx  SS=%04llx  error code=%016llx\n", (unsigned long long)f->rsp,
               (unsigned long long)f->ss, (unsigned long long)f->error_code);
    log_printf("SMY-OS:   RAX=%016llx  RBX=%016llx  RCX=%016llx\n", (unsigned long long)f->rax,
               (unsigned long long)f->rbx, (unsigned long long)f->rcx);
    log_printf("SMY-OS:   RDX=%016llx  RSI=%016llx  RDI=%016llx\n", (unsigned long long)f->rdx,
               (unsigned long long)f->rsi, (unsigned long long)f->rdi);
    log_printf("SMY-OS:   RBP=%016llx  R8 =%016llx  R9 =%016llx\n", (unsigned long long)f->rbp,
               (unsigned long long)f->r8, (unsigned long long)f->r9);
    log_printf("SMY-OS:   R10=%016llx  R11=%016llx  R12=%016llx\n", (unsigned long long)f->r10,
               (unsigned long long)f->r11, (unsigned long long)f->r12);
    log_printf("SMY-OS:   R13=%016llx  R14=%016llx  R15=%016llx\n", (unsigned long long)f->r13,
               (unsigned long long)f->r14, (unsigned long long)f->r15);
    log_printf("SMY-OS:   CR0=%016llx  CR3=%016llx  CR4=%016llx\n", (unsigned long long)read_cr0(),
               (unsigned long long)read_cr3(), (unsigned long long)read_cr4());
}

void exception_dispatch(struct interrupt_frame *frame)
{
    /* For #DF, CR2 still holds the address of a page fault that could not be
     * delivered, e.g. a push into a stack's guard page. */
    const uint64_t cr2 =
        frame->vector == EXC_PAGE_FAULT || frame->vector == EXC_DOUBLE_FAULT ? read_cr2() : 0;

    if (armed != NULL && armed->vector == frame->vector) {
        struct exception_expectation *e = armed;

        armed = NULL;
        e->error_code = frame->error_code;
        e->cr2 = cr2;
        e->fault_rip = frame->rip;
        e->hit = true;
        frame->rip = e->resume_rip;
        return;
    }

    if (!panic_begin("unhandled CPU exception %s (%s), vector %llu",
                     exception_mnemonic(frame->vector), exception_description(frame->vector),
                     (unsigned long long)frame->vector)) {
        panic_end();
    }
    dump_registers(frame);
    if (frame->vector == EXC_PAGE_FAULT) {
        dump_page_fault_details(frame->error_code, cr2);
        vmm_describe_fault(cr2);
    } else if (frame->vector == EXC_DOUBLE_FAULT) {
        uintptr_t lo;
        uintptr_t hi;

        log_printf("SMY-OS:   CR2=%016llx (last page-fault address)\n", (unsigned long long)cr2);
        if (vmm_stack_guard_hit(cr2, &lo, &hi) || vmm_stack_guard_hit(frame->rsp, &lo, &hi)) {
            vmm_describe_fault(vmm_stack_guard_hit(cr2, &lo, &hi) ? cr2 : frame->rsp);
        }
    }
    panic_print_stacktrace(frame->rip, frame->rbp);
    panic_end();
}
