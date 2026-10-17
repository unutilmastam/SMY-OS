/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

#include "core/panic.h"

#include "arch/cpu.h"
#include "core/log.h"

void panic(const char *message)
{
    log_write("SMY-OS: PANIC: ");
    log_write(message);
    log_write("\n");
    arch_halt_forever();
}
