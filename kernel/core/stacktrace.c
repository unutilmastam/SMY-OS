/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

#include "core/stacktrace.h"

#include <stdbool.h>

static bool frame_ok(uintptr_t fp, uintptr_t lo, uintptr_t hi)
{
    return fp % sizeof(uintptr_t) == 0 && fp >= lo && hi >= 2 * sizeof(uintptr_t) &&
           fp <= hi - 2 * sizeof(uintptr_t);
}

size_t stacktrace_walk(uintptr_t fp, uintptr_t lo, uintptr_t hi, uintptr_t *out, size_t max)
{
    size_t n = 0;

    while (n < max && fp != 0 && frame_ok(fp, lo, hi)) {
        const uintptr_t *frame = (const uintptr_t *)fp;
        const uintptr_t next = frame[0];
        const uintptr_t ret = frame[1];

        if (ret == 0) {
            break;
        }
        out[n++] = ret;
        if (next <= fp) {
            break; /* the chain must move up the stack; anything else is corrupt */
        }
        fp = next;
    }
    return n;
}
