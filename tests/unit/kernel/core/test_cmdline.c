/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

/* Host tests for kernel/core/cmdline.c. */

#include "core/cmdline.h"
#include "harness/smytest.h"

#include <string.h>

static int value_equals(const char *v, size_t n, const char *expected)
{
    return n == strlen(expected) && strncmp(v, expected, n) == 0;
}

SMY_TEST(cmdline_finds_key_value_and_bare_key)
{
    const char *line = "quiet smy.ktest=all  smy.loglevel=debug";
    const char *v = NULL;
    size_t n = 99;

    SMY_EXPECT(cmdline_find(line, "smy.ktest", &v, &n));
    SMY_EXPECT(value_equals(v, n, "all"));
    SMY_EXPECT(cmdline_find(line, "smy.loglevel", &v, &n));
    SMY_EXPECT(value_equals(v, n, "debug"));
    SMY_EXPECT(cmdline_find(line, "quiet", &v, &n));
    SMY_EXPECT_EQ_U64(n, 0);
    SMY_EXPECT(cmdline_find(line, "quiet", NULL, NULL));
}

SMY_TEST(cmdline_requires_whole_key)
{
    SMY_EXPECT(!cmdline_find("smy.ktestx=1", "smy.ktest", NULL, NULL));
    SMY_EXPECT(!cmdline_find("xsmy.ktest=1", "smy.ktest", NULL, NULL));
    SMY_EXPECT(!cmdline_find("smy.kt=1", "smy.ktest", NULL, NULL));
    SMY_EXPECT(!cmdline_find("", "smy.ktest", NULL, NULL));
    SMY_EXPECT(!cmdline_find(NULL, "smy.ktest", NULL, NULL));
    SMY_EXPECT(!cmdline_find("a=b", "", NULL, NULL));
}

SMY_TEST(cmdline_last_occurrence_wins_and_empty_value)
{
    const char *v = NULL;
    size_t n = 0;

    SMY_EXPECT(cmdline_find("k=1 k=22", "k", &v, &n));
    SMY_EXPECT(value_equals(v, n, "22"));
    SMY_EXPECT(cmdline_find("k= x", "k", &v, &n));
    SMY_EXPECT_EQ_U64(n, 0);
    SMY_EXPECT(cmdline_find("\tk=v\n", "k", &v, &n));
    SMY_EXPECT(value_equals(v, n, "v"));
}

SMY_TEST(cmdline_value_is_compares_exactly)
{
    const char *line = "smy.ktest=all";

    SMY_EXPECT(cmdline_value_is(line + 10, 3, "all"));
    SMY_EXPECT(!cmdline_value_is(line + 10, 3, "al"));
    SMY_EXPECT(!cmdline_value_is(line + 10, 2, "all"));
    SMY_EXPECT(cmdline_value_is(line, 0, ""));
    SMY_EXPECT(!cmdline_value_is(NULL, 0, ""));
}
