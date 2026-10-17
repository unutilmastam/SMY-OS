/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

/*
 * Architecture-independent CPU interface. Generic kernel code (core/, and
 * drivers for hardware that is not tied to one architecture) includes only
 * this header; each architecture implements it under arch/<name>/
 * (docs/architecture/hardware.md, "Portability rule").
 */

#ifndef SMY_KERNEL_ARCH_CPU_H
#define SMY_KERNEL_ARCH_CPU_H

#include "core/compiler.h"

#include <stdbool.h>
#include <stdint.h>

/* Install this CPU's descriptor tables and exception handlers. */
void arch_cpu_init(void);

/* Disable interrupts and stop this CPU permanently. */
SMY_NORETURN void arch_halt_forever(void);

/*
 * End a self-test run and report the result to the emulator (QEMU's
 * isa-debug-exit device on x86). Only called in ktest mode; if no such
 * device exists the CPU halts.
 */
SMY_NORETURN void arch_test_exit(bool success);

/* If `addr` lies on a known kernel stack, return that stack's bounds. */
bool arch_stack_bounds(uintptr_t addr, uintptr_t *lo, uintptr_t *hi);

#endif /* SMY_KERNEL_ARCH_CPU_H */
