/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

/*
 * The only place where compiler extensions are spelled out (ADR 0002).
 * Kernel code uses these macros instead of raw __attribute__ syntax.
 */

#ifndef SMY_KERNEL_CORE_COMPILER_H
#define SMY_KERNEL_CORE_COMPILER_H

#define SMY_NORETURN _Noreturn
#define SMY_USED __attribute__((used))
#define SMY_SECTION(name) __attribute__((section(name)))
#define SMY_ALIGNED(n) __attribute__((aligned(n)))
#define SMY_LIKELY(x) __builtin_expect(!!(x), 1)
#define SMY_UNLIKELY(x) __builtin_expect(!!(x), 0)
#define SMY_PACKED __attribute__((packed))
/* printf-style format checking for our own formatter (core/fmt.h), which
 * implements a subset of the standard conversions with the same meaning. */
#define SMY_PRINTF(fmt_index, first_arg) __attribute__((format(printf, fmt_index, first_arg)))

#endif /* SMY_KERNEL_CORE_COMPILER_H */
