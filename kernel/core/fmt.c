/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

#include "core/fmt.h"

size_t fmt_u64(char *buf, size_t size, uint64_t value, unsigned base)
{
    static const char digits[] = "0123456789abcdef";
    char tmp[FMT_U64_BUF_SIZE];
    size_t len = 0;

    if (buf == NULL || size == 0) {
        return 0;
    }
    buf[0] = '\0';
    if (base < 2 || base > 16) {
        return 0;
    }

    /* Digits come out least significant first; reverse them afterwards. */
    do {
        tmp[len++] = digits[value % base];
        value /= base;
    } while (value != 0);

    if (len + 1 > size) {
        return 0;
    }
    for (size_t i = 0; i < len; i++) {
        buf[i] = tmp[len - 1 - i];
    }
    buf[len] = '\0';
    return len;
}
