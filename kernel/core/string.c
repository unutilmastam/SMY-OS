/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

/*
 * Byte-at-a-time implementations. They are simple on purpose: correctness
 * first; word-sized or `rep movsb` variants can replace them once there are
 * benchmarks (Phase 8+) showing they matter.
 */

#include "core/string.h"

#include <stdint.h>

void *SMY_KSYM(memcpy)(void *restrict dst, const void *restrict src, size_t n)
{
    unsigned char *d = dst;
    const unsigned char *s = src;

    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dst;
}

void *SMY_KSYM(memmove)(void *dst, const void *src, size_t n)
{
    unsigned char *d = dst;
    const unsigned char *s = src;

    if (d == s || n == 0) {
        return dst;
    }
    /* Compare as integers: relational operators on unrelated pointers are
     * undefined behaviour in C. */
    if ((uintptr_t)d < (uintptr_t)s) {
        for (size_t i = 0; i < n; i++) {
            d[i] = s[i];
        }
    } else {
        for (size_t i = n; i > 0; i--) {
            d[i - 1] = s[i - 1];
        }
    }
    return dst;
}

void *SMY_KSYM(memset)(void *dst, int value, size_t n)
{
    unsigned char *d = dst;
    const unsigned char byte = (unsigned char)value;

    for (size_t i = 0; i < n; i++) {
        d[i] = byte;
    }
    return dst;
}

int SMY_KSYM(memcmp)(const void *a, const void *b, size_t n)
{
    const unsigned char *pa = a;
    const unsigned char *pb = b;

    for (size_t i = 0; i < n; i++) {
        if (pa[i] != pb[i]) {
            return pa[i] < pb[i] ? -1 : 1;
        }
    }
    return 0;
}

size_t SMY_KSYM(strlen)(const char *s)
{
    size_t len = 0;

    while (s[len] != '\0') {
        len++;
    }
    return len;
}
