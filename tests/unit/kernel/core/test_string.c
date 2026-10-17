/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

/* Host tests for kernel/core/string.c (built with SMY_HOST_TEST). */

#include "core/string.h"
#include "harness/smytest.h"

#include <stdint.h>
#include <string.h>

#define BUF_SIZE 64

/* Deterministic, non-trivial byte pattern so off-by-one copies are visible. */
static void fill_pattern(unsigned char *buf, size_t n, unsigned char seed)
{
    for (size_t i = 0; i < n; i++) {
        buf[i] = (unsigned char)(seed + i * 7u);
    }
}

SMY_TEST(memcpy_copies_every_length_and_alignment)
{
    unsigned char src[BUF_SIZE];
    unsigned char dst[BUF_SIZE];
    unsigned char expected[BUF_SIZE];

    fill_pattern(src, sizeof src, 3);
    for (size_t off = 0; off < 8; off++) {
        for (size_t len = 0; off + len <= 32; len++) {
            memset(dst, 0xAA, sizeof dst);
            memset(expected, 0xAA, sizeof expected);
            memcpy(expected + off, src + off, len);

            SMY_EXPECT_EQ_PTR(SMY_KSYM(memcpy)(dst + off, src + off, len), dst + off);
            SMY_EXPECT(memcmp(dst, expected, sizeof dst) == 0);
        }
    }
}

SMY_TEST(memmove_handles_forward_overlap)
{
    unsigned char buf[BUF_SIZE];
    unsigned char expected[BUF_SIZE];

    fill_pattern(buf, sizeof buf, 11);
    memcpy(expected, buf, sizeof buf);
    memmove(expected + 5, expected, 20);

    SMY_EXPECT_EQ_PTR(SMY_KSYM(memmove)(buf + 5, buf, 20), buf + 5);
    SMY_EXPECT(memcmp(buf, expected, sizeof buf) == 0);
}

SMY_TEST(memmove_handles_backward_overlap)
{
    unsigned char buf[BUF_SIZE];
    unsigned char expected[BUF_SIZE];

    fill_pattern(buf, sizeof buf, 23);
    memcpy(expected, buf, sizeof buf);
    memmove(expected, expected + 5, 20);

    SMY_EXPECT_EQ_PTR(SMY_KSYM(memmove)(buf, buf + 5, 20), buf);
    SMY_EXPECT(memcmp(buf, expected, sizeof buf) == 0);
}

SMY_TEST(memmove_same_pointer_and_zero_length_change_nothing)
{
    unsigned char buf[BUF_SIZE];
    unsigned char expected[BUF_SIZE];

    fill_pattern(buf, sizeof buf, 5);
    memcpy(expected, buf, sizeof buf);

    SMY_EXPECT_EQ_PTR(SMY_KSYM(memmove)(buf, buf, sizeof buf), buf);
    SMY_EXPECT_EQ_PTR(SMY_KSYM(memmove)(buf + 1, buf, 0), buf + 1);
    SMY_EXPECT(memcmp(buf, expected, sizeof buf) == 0);
}

SMY_TEST(memset_fills_only_the_requested_range)
{
    unsigned char buf[BUF_SIZE];

    memset(buf, 0x11, sizeof buf);
    SMY_EXPECT_EQ_PTR(SMY_KSYM(memset)(buf + 3, 0x7F, 10), buf + 3);
    for (size_t i = 0; i < sizeof buf; i++) {
        const unsigned expected = (i >= 3 && i < 13) ? 0x7Fu : 0x11u;
        SMY_EXPECT_EQ_U64(buf[i], expected);
    }
}

SMY_TEST(memset_uses_only_the_low_byte_of_value)
{
    unsigned char buf[4];

    /* Deliberately out of range: memset must use only the low byte. */
    // cppcheck-suppress memsetValueOutOfRange
    SMY_KSYM(memset)(buf, 0x1234, sizeof buf);
    for (size_t i = 0; i < sizeof buf; i++) {
        SMY_EXPECT_EQ_U64(buf[i], 0x34);
    }
    SMY_KSYM(memset)(buf, -1, sizeof buf);
    for (size_t i = 0; i < sizeof buf; i++) {
        SMY_EXPECT_EQ_U64(buf[i], 0xFF);
    }
}

SMY_TEST(memcmp_orders_bytes_as_unsigned)
{
    const unsigned char low[] = {0x00, 0x01, 0x7F};
    const unsigned char high[] = {0x00, 0x01, 0x80};

    SMY_EXPECT(SMY_KSYM(memcmp)(low, high, sizeof low) < 0);
    SMY_EXPECT(SMY_KSYM(memcmp)(high, low, sizeof low) > 0);
    /* Deliberately the same buffer twice: equal input must compare equal. */
    // cppcheck-suppress stringCompare
    SMY_EXPECT(SMY_KSYM(memcmp)(low, low, sizeof low) == 0);
    /* Only the first n bytes count. */
    SMY_EXPECT(SMY_KSYM(memcmp)(low, high, 2) == 0);
    SMY_EXPECT(SMY_KSYM(memcmp)(low, high, 0) == 0);
}

SMY_TEST(strlen_counts_bytes_before_terminator)
{
    SMY_EXPECT_EQ_U64(SMY_KSYM(strlen)(""), 0);
    SMY_EXPECT_EQ_U64(SMY_KSYM(strlen)("SMY-OS"), 6);
    /* UTF-8 text: strlen counts bytes, not characters ("o'zbek" with U+02BB). */
    SMY_EXPECT_EQ_U64(SMY_KSYM(strlen)("o\xCA\xBBzbek"), 7);
}
