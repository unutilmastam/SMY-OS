/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

#include "arch/cpu.h"

#include "arch/x86_64/io.h"

void arch_halt_forever(void)
{
    cpu_halt_forever();
}
