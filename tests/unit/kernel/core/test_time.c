/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

/* Host tests for the counter arithmetic in kernel/core/time.c. */

#include "core/time.h"
#include "harness/smytest.h"

#define HPET_QEMU_FS 10000000ull /* 100 MHz: 10 ns */
#define HPET_ICH_FS 69841279ull  /* 14.31818 MHz */

SMY_TEST(time_counter_to_ns_is_exact_for_common_periods)
{
    SMY_EXPECT_EQ_U64(time_counter_to_ns(0, HPET_QEMU_FS), 0);
    SMY_EXPECT_EQ_U64(time_counter_to_ns(1, HPET_QEMU_FS), 10);
    SMY_EXPECT_EQ_U64(time_counter_to_ns(100000000, HPET_QEMU_FS), NS_PER_S);
    /* 14.31818 MHz: one second of ticks is within 1 ns of 1e9 ns. */
    {
        const uint64_t ns = time_counter_to_ns(14318180, HPET_ICH_FS);

        SMY_EXPECT(ns >= NS_PER_S - 1000 && ns <= NS_PER_S + 1000);
    }
}

SMY_TEST(time_counter_to_ns_does_not_overflow)
{
    /* A 64-bit counter near its limit at 10 ns per tick: the naive
     * count * period_fs would overflow at once. */
    const uint64_t big = 0xFFFFFFFFFFFFFFFFull;
    const uint64_t ns = time_counter_to_ns(big, HPET_QEMU_FS);

    SMY_EXPECT_EQ_U64(ns, big / 1000000ull * HPET_QEMU_FS + (big % 1000000ull) * 10);
    /* Monotonic in the counter. */
    SMY_EXPECT(time_counter_to_ns(123456789012ull, HPET_ICH_FS) <
               time_counter_to_ns(123456789013ull, HPET_ICH_FS) + 1);
}

SMY_TEST(time_ns_to_counter_rounds_up_and_inverts)
{
    SMY_EXPECT_EQ_U64(time_ns_to_counter(10, HPET_QEMU_FS), 1);
    SMY_EXPECT_EQ_U64(time_ns_to_counter(11, HPET_QEMU_FS), 2);
    SMY_EXPECT_EQ_U64(time_ns_to_counter(NS_PER_MS, HPET_QEMU_FS), 100000);
    for (uint64_t ns = 0; ns < 5000000; ns += 777) {
        const uint64_t ticks = time_ns_to_counter(ns, HPET_ICH_FS);

        if (time_counter_to_ns(ticks, HPET_ICH_FS) < ns) {
            SMY_EXPECT(time_counter_to_ns(ticks, HPET_ICH_FS) >= ns);
            break;
        }
    }
}
