/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

#include "core/time.h"

uint64_t time_counter_to_ns(uint64_t count, uint64_t period_fs)
{
    /* count = q * 1e6 + r: q * period_fs is exact; r * period_fs < 1e6 * 1e9. */
    const uint64_t q = count / FS_PER_NS;
    const uint64_t r = count % FS_PER_NS;

    return q * period_fs + r * period_fs / FS_PER_NS;
}

uint64_t time_ns_to_counter(uint64_t ns, uint64_t period_fs)
{
    /* ns * 1e6 / period_fs, split the same way: ns = q * period_fs + r. */
    const uint64_t q = ns / period_fs;
    const uint64_t r = ns % period_fs;

    return q * FS_PER_NS + (r * FS_PER_NS + period_fs - 1) / period_fs;
}
