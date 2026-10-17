/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

/*
 * Kernel memory and string primitives.
 *
 * Inside the kernel these are the real `memcpy`, `memset`, ... symbols: the
 * compiler may emit calls to them even in freestanding code, so the kernel
 * must provide them. Host unit tests compile the same source with
 * SMY_HOST_TEST defined, which renames every function to `smy_host_<name>`
 * so it does not collide with the host C library.
 */

#ifndef SMY_KERNEL_CORE_STRING_H
#define SMY_KERNEL_CORE_STRING_H

#include <stddef.h>

#ifdef SMY_HOST_TEST
#define SMY_KSYM(name) smy_host_##name
#else
#define SMY_KSYM(name) name
#endif

void *SMY_KSYM(memcpy)(void *restrict dst, const void *restrict src, size_t n);
void *SMY_KSYM(memmove)(void *dst, const void *src, size_t n);
void *SMY_KSYM(memset)(void *dst, int value, size_t n);
int SMY_KSYM(memcmp)(const void *a, const void *b, size_t n);
size_t SMY_KSYM(strlen)(const char *s);

#endif /* SMY_KERNEL_CORE_STRING_H */
