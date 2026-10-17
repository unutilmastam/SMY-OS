/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

/*
 * Text formatting without a C library (host-testable).
 *
 * fmt_vformat() implements this printf subset, with the standard meaning:
 *   conversions  %d %i %u %x %X %p %c %s %%
 *   flags        '-' (left-align), '0' (zero-pad numbers)
 *   width        decimal digits, e.g. %08x, %-12s
 *   precision    ".N" or ".*" — honoured for %s (maximum characters, so
 *                strings without a NUL can be printed); ignored for integers
 *   length       hh h l ll z (integers)
 * Anything else ('*' width, floating point, %n) is not supported and is
 * printed literally, so a bad format string can never write memory.
 */

#ifndef SMY_KERNEL_CORE_FMT_H
#define SMY_KERNEL_CORE_FMT_H

#include "core/compiler.h"

#include <stdarg.h>
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

/* Output callback: receives one character at a time. */
typedef void (*fmt_putc_fn)(char c, void *ctx);

/* Format into `putc`; returns the number of characters produced. */
size_t fmt_vformat(fmt_putc_fn putc, void *ctx, const char *fmt, va_list args);

/*
 * Format into `buf` (always NUL-terminated when size > 0, truncated if
 * needed). Returns the length the full output would have had, like
 * snprintf, so `ret >= size` means it was truncated.
 */
size_t fmt_snprintf(char *buf, size_t size, const char *fmt, ...) SMY_PRINTF(3, 4);
size_t fmt_vsnprintf(char *buf, size_t size, const char *fmt, va_list args);

#endif /* SMY_KERNEL_CORE_FMT_H */
