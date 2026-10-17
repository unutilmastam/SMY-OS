/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

/* Host tests for kernel/core/fmt.c. */

#include "core/fmt.h"
#include "harness/smytest.h"

#include <string.h>

SMY_TEST(fmt_u64_decimal)
{
    char buf[FMT_U64_BUF_SIZE];

    SMY_EXPECT_EQ_U64(fmt_u64(buf, sizeof buf, 0, 10), 1);
    SMY_EXPECT(strcmp(buf, "0") == 0);
    SMY_EXPECT_EQ_U64(fmt_u64(buf, sizeof buf, 1280, 10), 4);
    SMY_EXPECT(strcmp(buf, "1280") == 0);
    SMY_EXPECT_EQ_U64(fmt_u64(buf, sizeof buf, UINT64_MAX, 10), 20);
    SMY_EXPECT(strcmp(buf, "18446744073709551615") == 0);
}

SMY_TEST(fmt_u64_hex_and_binary)
{
    char buf[FMT_U64_BUF_SIZE];

    SMY_EXPECT_EQ_U64(fmt_u64(buf, sizeof buf, 0xffffffff80000000u, 16), 16);
    SMY_EXPECT(strcmp(buf, "ffffffff80000000") == 0);
    SMY_EXPECT_EQ_U64(fmt_u64(buf, sizeof buf, 5, 2), 3);
    SMY_EXPECT(strcmp(buf, "101") == 0);
    /* The largest output: 64 binary digits must fit FMT_U64_BUF_SIZE. */
    SMY_EXPECT_EQ_U64(fmt_u64(buf, sizeof buf, UINT64_MAX, 2), 64);
    SMY_EXPECT_EQ_U64(strlen(buf), 64);
}

SMY_TEST(fmt_u64_rejects_bad_base)
{
    char buf[8] = "xxxxxxx";

    SMY_EXPECT_EQ_U64(fmt_u64(buf, sizeof buf, 42, 1), 0);
    SMY_EXPECT(buf[0] == '\0');
    SMY_EXPECT_EQ_U64(fmt_u64(buf, sizeof buf, 42, 17), 0);
    SMY_EXPECT(buf[0] == '\0');
}

SMY_TEST(fmt_u64_rejects_small_buffer_without_overflow)
{
    char buf[4] = "zzz";

    /* "1234" needs 5 bytes including NUL; ASan would catch an overflow. */
    SMY_EXPECT_EQ_U64(fmt_u64(buf, sizeof buf, 1234, 10), 0);
    SMY_EXPECT(buf[0] == '\0');
    SMY_EXPECT_EQ_U64(fmt_u64(buf, sizeof buf, 123, 10), 3);
    SMY_EXPECT(strcmp(buf, "123") == 0);
    SMY_EXPECT_EQ_U64(fmt_u64(NULL, 4, 1, 10), 0);
    SMY_EXPECT_EQ_U64(fmt_u64(buf, 0, 1, 10), 0);
}
