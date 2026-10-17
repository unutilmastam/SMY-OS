/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

/* Number-to-text conversion without a C library (host-testable). */

#ifndef SMY_KERNEL_CORE_FMT_H
#define SMY_KERNEL_CORE_FMT_H

#include <stddef.h>
#include <stdint.h>

/* Enough for UINT64_MAX in base 2 plus the terminating NUL. */
#define FMT_U64_BUF_SIZE 65

/*
 * Write `value` in `base` (2..16, lower-case digits, no prefix) into `buf`
 * as a NUL-terminated string. Returns the number of digits written, or 0 if
 * `base` is invalid or `buf` is too small (then `buf` is left as "" when
 * size > 0).
 */
size_t fmt_u64(char *buf, size_t size, uint64_t value, unsigned base);

#endif /* SMY_KERNEL_CORE_FMT_H */
