/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

#include "core/panic.h"

#include "arch/cpu.h"
#include "core/log.h"
#include "core/stacktrace.h"

#include <stdarg.h>
#include <stddef.h>

#define PANIC_TRACE_DEPTH 16

static bool panicking;
static bool test_mode;

void panic_set_test_mode(bool enabled)
{
    test_mode = enabled;
}

static bool vpanic_begin(const char *fmt, va_list args)
{
    if (panicking) {
        log_write("\nSMY-OS: PANIC: nested panic (fault while reporting a panic)\n");
        return false;
    }
    panicking = true;
    log_write("\nSMY-OS: PANIC: ");
    log_vprintf(fmt, args);
    log_write("\n");
    return true;
}

bool panic_begin(const char *fmt, ...)
{
    va_list args;
    bool first;

    va_start(args, fmt);
    first = vpanic_begin(fmt, args);
    va_end(args);
    return first;
}

void panic_print_stacktrace(uintptr_t first_pc, uintptr_t fp)
{
    uintptr_t frames[PANIC_TRACE_DEPTH];
    uintptr_t lo;
    uintptr_t hi;
    size_t count = 0;

    log_write("SMY-OS: stack trace:\n");
    if (first_pc != 0) {
        log_printf("SMY-OS:   #0  %p\n", (void *)first_pc);
    }
    if (arch_stack_bounds(fp, &lo, &hi)) {
        count = stacktrace_walk(fp, lo, hi, frames, PANIC_TRACE_DEPTH);
    } else {
        log_printf("SMY-OS:   (frame pointer %p is not on a known kernel stack)\n", (void *)fp);
    }
    for (size_t i = 0; i < count; i++) {
        log_printf("SMY-OS:   #%zu  %p\n", i + (first_pc != 0 ? 1u : 0u), (void *)frames[i]);
    }
    log_write("SMY-OS: (resolve addresses with: llvm-addr2line -f -e "
              "build/kernel/smy-kernel.elf <address>)\n");
}

void panic_end(void)
{
    if (test_mode) {
        log_write("SMY-OS: ktest: run aborted by panic\n");
        arch_test_exit(false);
    }
    log_write("SMY-OS: system halted\n");
    arch_halt_forever();
}

void panic(const char *fmt, ...)
{
    va_list args;
    bool first;

    va_start(args, fmt);
    first = vpanic_begin(fmt, args);
    va_end(args);
    if (first) {
        panic_print_stacktrace(0, (uintptr_t)__builtin_frame_address(0));
    }
    panic_end();
}
