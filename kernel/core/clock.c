/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

/* Kernel clock on top of the architecture's counter (core/time.h). */

#include "arch/platform.h"
#include "core/time.h"

uint64_t clock_monotonic_ns(void)
{
    return time_counter_to_ns(arch_clock_counter(), arch_clock_period_fs());
}

uint64_t clock_ticks(void)
{
    return arch_timer_ticks();
}

void clock_sleep_ns(uint64_t ns)
{
    const uint64_t end = clock_monotonic_ns() + ns;

    while (clock_monotonic_ns() < end) {
        arch_wait_for_interrupt();
    }
}
