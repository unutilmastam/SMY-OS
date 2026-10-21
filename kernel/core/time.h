/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

/*
 * Kernel time: the monotonic clock and the periodic tick
 * (docs/kernel/interrupts.md). Architecture code supplies a counter
 * (arch/timer.h); this file holds the overflow-safe arithmetic, which is
 * host-tested.
 */

#ifndef SMY_KERNEL_CORE_TIME_H
#define SMY_KERNEL_CORE_TIME_H

#include <stdint.h>

#define NS_PER_MS 1000000ull
#define NS_PER_S 1000000000ull
#define FS_PER_NS 1000000ull

/*
 * Convert a counter value to nanoseconds, the counter ticking every
 * `period_fs` femtoseconds (HPET: 10 ns .. 100 ns = 1e7 .. 1e8 fs). Exact
 * and overflow-free for any 64-bit count while period_fs <= 1e9, without
 * 128-bit division (the kernel does not link a compiler runtime).
 */
uint64_t time_counter_to_ns(uint64_t count, uint64_t period_fs);

/* Counter ticks that cover `ns` nanoseconds (rounded up). */
uint64_t time_ns_to_counter(uint64_t ns, uint64_t period_fs);

/* Nanoseconds since the clock started; never decreases. */
uint64_t clock_monotonic_ns(void);

/* Periodic timer interrupts since it was started. */
uint64_t clock_ticks(void);

/* Wait at least `ns` (halts between interrupts; interrupts must be on). */
void clock_sleep_ns(uint64_t ns);

#endif /* SMY_KERNEL_CORE_TIME_H */
