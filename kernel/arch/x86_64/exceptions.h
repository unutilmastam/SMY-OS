/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

/* CPU exception handling (vectors 0-31). Output format: docs/kernel/exceptions.md. */

#ifndef SMY_KERNEL_ARCH_X86_64_EXCEPTIONS_H
#define SMY_KERNEL_ARCH_X86_64_EXCEPTIONS_H

#include <stdbool.h>
#include <stdint.h>

/* Register state saved by isr.S; field order is the stack layout. */
struct interrupt_frame {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t vector;
    uint64_t error_code;
    uint64_t rip, cs, rflags, rsp, ss; /* pushed by the CPU */
};

_Static_assert(sizeof(struct interrupt_frame) == 22u * sizeof(uint64_t),
               "frame layout must match isr.S");

enum {
    EXC_DIVIDE_ERROR = 0,
    EXC_DEBUG = 1,
    EXC_NMI = 2,
    EXC_BREAKPOINT = 3,
    EXC_INVALID_OPCODE = 6,
    EXC_DOUBLE_FAULT = 8,
    EXC_GENERAL_PROTECTION = 13,
    EXC_PAGE_FAULT = 14,
    EXC_MACHINE_CHECK = 18,
};

/* Short mnemonic ("#PF") and description for a vector. */
const char *exception_mnemonic(uint64_t vector);
const char *exception_description(uint64_t vector);

/*
 * Expected-exception hook for ktests. While armed, an exception with the
 * given vector is recorded (hit, error code, CR2, faulting RIP), execution
 * resumes at `resume_rip`, and the hook disarms itself. Any other exception
 * still panics. Kernel code outside ktests never arms it.
 */
struct exception_expectation {
    uint64_t vector;
    uintptr_t resume_rip;
    volatile bool hit;
    volatile uint64_t error_code;
    volatile uint64_t cr2;
    volatile uint64_t fault_rip;
};

void exceptions_expect(struct exception_expectation *expectation);

/* Called from isr.S. */
void exception_dispatch(struct interrupt_frame *frame);

#endif /* SMY_KERNEL_ARCH_X86_64_EXCEPTIONS_H */
