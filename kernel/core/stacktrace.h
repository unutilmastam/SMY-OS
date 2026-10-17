/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

/*
 * Frame-pointer stack walking (host-testable). The kernel is built with
 * -fno-omit-frame-pointer, so every frame starts with
 *   [fp + 0] = caller's frame pointer
 *   [fp + 8] = return address
 * The walker only follows frames that lie inside [lo, hi) — the bounds of a
 * known kernel stack — are 8-byte aligned and move strictly towards higher
 * addresses, so a corrupt chain stops the walk instead of faulting.
 */

#ifndef SMY_KERNEL_CORE_STACKTRACE_H
#define SMY_KERNEL_CORE_STACKTRACE_H

#include <stddef.h>
#include <stdint.h>

/* Store up to `max` return addresses into `out`; returns how many. */
size_t stacktrace_walk(uintptr_t fp, uintptr_t lo, uintptr_t hi, uintptr_t *out, size_t max);

#endif /* SMY_KERNEL_CORE_STACKTRACE_H */
