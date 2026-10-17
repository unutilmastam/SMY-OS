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

#endif /* SMY_KERNEL_CORE_COMPILER_H */
