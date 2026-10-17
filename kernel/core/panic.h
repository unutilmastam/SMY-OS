/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

#ifndef SMY_KERNEL_CORE_PANIC_H
#define SMY_KERNEL_CORE_PANIC_H

#include "core/compiler.h"

#include <stdbool.h>
#include <stdint.h>

/*
 * Stop the system after an unrecoverable error. Prints
 * "SMY-OS: PANIC: <message>" (boot tests treat that line as a failure) and a
 * stack trace of the caller, then halts — or, in ktest mode, ends the test
 * run as failed. Output format: docs/kernel/exceptions.md.
 */
SMY_NORETURN void panic(const char *fmt, ...) SMY_PRINTF(1, 2);

/*
 * Building blocks for panics with extra context (CPU exceptions):
 * panic_begin() prints the PANIC line and returns false if a panic is already
 * in progress (the caller must then call panic_end() at once, without
 * touching anything that may have caused the first panic);
 * panic_print_stacktrace() prints "SMY-OS: stack trace:" with `first_pc`
 * followed by the frames reachable from `fp`; panic_end() stops.
 */
bool panic_begin(const char *fmt, ...) SMY_PRINTF(1, 2);
void panic_print_stacktrace(uintptr_t first_pc, uintptr_t fp);
SMY_NORETURN void panic_end(void);

/* In ktest mode a panic ends the test run (arch_test_exit) instead of halting. */
void panic_set_test_mode(bool enabled);

#endif /* SMY_KERNEL_CORE_PANIC_H */
