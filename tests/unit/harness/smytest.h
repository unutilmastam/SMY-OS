/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

/*
 * smytest: minimal in-repo unit-test harness for host tests.
 *
 * A test is declared with SMY_TEST(name) { ... } and registers itself before
 * main() through a constructor. Checks record a failure and continue, so one
 * run reports every failing check of a test. The runner (smytest.c) exits
 * non-zero if any test failed or if no test was registered at all.
 */

#ifndef SMY_TESTS_UNIT_HARNESS_SMYTEST_H
#define SMY_TESTS_UNIT_HARNESS_SMYTEST_H

#include <stddef.h>
#include <stdint.h>

typedef void (*smy_test_fn)(void);

void smy_test_register(const char *name, smy_test_fn fn);
void smy_test_fail(const char *file, int line, const char *fmt, ...)
    __attribute__((format(printf, 3, 4)));

#define SMY_TEST(name)                                                                             \
    static void name(void);                                                                        \
    __attribute__((constructor)) static void name##_register(void)                                 \
    {                                                                                              \
        smy_test_register(#name, name);                                                            \
    }                                                                                              \
    static void name(void)

#define SMY_EXPECT(cond)                                                                           \
    do {                                                                                           \
        if (!(cond)) {                                                                             \
            smy_test_fail(__FILE__, __LINE__, "expected: %s", #cond);                              \
        }                                                                                          \
    } while (0)

#define SMY_EXPECT_EQ_U64(actual, expected)                                                        \
    do {                                                                                           \
        const uint64_t smy_a_ = (uint64_t)(actual);                                                \
        const uint64_t smy_e_ = (uint64_t)(expected);                                              \
        if (smy_a_ != smy_e_) {                                                                    \
            smy_test_fail(__FILE__, __LINE__, "%s == %llu, expected %llu", #actual,                \
                          (unsigned long long)smy_a_, (unsigned long long)smy_e_);                 \
        }                                                                                          \
    } while (0)

#define SMY_EXPECT_EQ_PTR(actual, expected)                                                        \
    do {                                                                                           \
        const void *smy_a_ = (actual);                                                             \
        const void *smy_e_ = (expected);                                                           \
        if (smy_a_ != smy_e_) {                                                                    \
            smy_test_fail(__FILE__, __LINE__, "%s == %p, expected %p", #actual, smy_a_, smy_e_);   \
        }                                                                                          \
    } while (0)

#endif /* SMY_TESTS_UNIT_HARNESS_SMYTEST_H */
