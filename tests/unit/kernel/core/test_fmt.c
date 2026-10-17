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

#define CHECK_FMT(expected, ...)                                                                   \
    do {                                                                                           \
        char out_[128];                                                                            \
        const size_t n_ = fmt_snprintf(out_, sizeof out_, __VA_ARGS__);                            \
        if (strcmp(out_, (expected)) != 0 || n_ != strlen(expected)) {                             \
            smy_test_fail(__FILE__, __LINE__, "got \"%s\" (%zu), expected \"%s\"", out_, n_,       \
                          (expected));                                                             \
        }                                                                                          \
    } while (0)

SMY_TEST(fmt_snprintf_matches_printf_for_supported_conversions)
{
    CHECK_FMT("plain text", "plain text");
    CHECK_FMT("100%", "100%%");
    CHECK_FMT("-42 42 0", "%d %i %u", -42, 42, 0u);
    CHECK_FMT("-2147483648", "%d", -2147483647 - 1);
    CHECK_FMT("-9223372036854775808", "%lld", (long long)INT64_MIN);
    CHECK_FMT("18446744073709551615", "%llu", (unsigned long long)UINT64_MAX);
    CHECK_FMT("beef BEEF", "%x %X", 0xbeefu, 0xbeefu);
    CHECK_FMT("ff", "%hhx", 0x1ffu);
    CHECK_FMT("ffff", "%hx", 0x1ffffu);
    CHECK_FMT("123456789012", "%zu", (size_t)123456789012u);
    CHECK_FMT("x=A", "x=%c", 'A');
    CHECK_FMT("SMY-OS", "%s", "SMY-OS");
    CHECK_FMT("(null)", "%s", (const char *)NULL);
}

SMY_TEST(fmt_snprintf_width_flags_and_precision)
{
    CHECK_FMT("00000000beef", "%012x", 0xbeefu);
    CHECK_FMT("   42", "%5d", 42);
    CHECK_FMT("42   |", "%-5d|", 42);
    CHECK_FMT("-0042", "%05d", -42);
    CHECK_FMT("ab   |", "%-5s|", "ab");
    CHECK_FMT("   ab", "%5s", "ab");
    CHECK_FMT("SMY", "%.3s", "SMY-OS");
    CHECK_FMT("SM", "%.*s", 2, "SMY-OS");
    CHECK_FMT("SMY-OS", "%.*s", -1, "SMY-OS");
    CHECK_FMT("0xffffffff80001000", "%p", (void *)0xffffffff80001000u);
    CHECK_FMT("0x0000000000000010", "%p", (void *)0x10u);
}

SMY_TEST(fmt_snprintf_reads_only_given_length_with_precision)
{
    /* No NUL terminator: precision must stop the read (ASan checks it). */
    const char not_terminated[3] = {'a', 'b', 'c'};

    CHECK_FMT("abc", "%.3s", not_terminated);
}

SMY_TEST(fmt_snprintf_truncates_and_reports_full_length)
{
    char buf[5];

    SMY_EXPECT_EQ_U64(fmt_snprintf(buf, sizeof buf, "%s", "SMY-OS"), 6);
    SMY_EXPECT(strcmp(buf, "SMY-") == 0);
    SMY_EXPECT_EQ_U64(fmt_snprintf(buf, 1, "abc"), 3);
    SMY_EXPECT(buf[0] == '\0');
    SMY_EXPECT_EQ_U64(fmt_snprintf(NULL, 0, "%d", 12345), 5);
}

SMY_TEST(fmt_unsupported_conversions_are_printed_literally)
{
    /* Called through a pointer so -Wformat does not reject the deliberately
     * unsupported conversions; none of them consumes an argument. */
    size_t (*const unchecked)(char *, size_t, const char *, ...) = fmt_snprintf;
    char buf[32];

    unchecked(buf, sizeof buf, "%f %n %q");
    SMY_EXPECT(strcmp(buf, "%f %n %q") == 0);
    unchecked(buf, sizeof buf, "end%");
    SMY_EXPECT(strcmp(buf, "end%") == 0);
}
