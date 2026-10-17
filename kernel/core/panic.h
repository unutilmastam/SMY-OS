/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

#ifndef SMY_KERNEL_CORE_PANIC_H
#define SMY_KERNEL_CORE_PANIC_H

#include "core/compiler.h"

/*
 * Stop the system after an unrecoverable error. Prints
 * "SMY-OS: PANIC: <message>" to the serial log (boot tests treat that line
 * as a failure) and halts this CPU. Register dumps and stack traces arrive
 * with exception handling in Phase 3.
 */
SMY_NORETURN void panic(const char *message);

#endif /* SMY_KERNEL_CORE_PANIC_H */
