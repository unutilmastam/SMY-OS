/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

#include "harness/smytest.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define SMY_TEST_MAX 1024

struct smy_test {
    const char *name;
    smy_test_fn fn;
};

static struct smy_test tests[SMY_TEST_MAX];
static size_t test_count;
static size_t current_failures;
static int registry_overflow;

void smy_test_register(const char *name, smy_test_fn fn)
{
    if (test_count >= SMY_TEST_MAX) {
        registry_overflow = 1;
        return;
    }
    tests[test_count].name = name;
    tests[test_count].fn = fn;
    test_count++;
}

void smy_test_fail(const char *file, int line, const char *fmt, ...)
{
    va_list args;

    current_failures++;
    fprintf(stderr, "  %s:%d: ", file, line);
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fputc('\n', stderr);
}

/* Usage: <binary> [substring]  — runs only tests whose name contains it. */
int main(int argc, char **argv)
{
    const char *filter = argc > 1 ? argv[1] : NULL;
    size_t run = 0;
    size_t failed = 0;

    if (registry_overflow) {
        fprintf(stderr, "smytest: more than %d tests registered\n", SMY_TEST_MAX);
        return 1;
    }
    for (size_t i = 0; i < test_count; i++) {
        if (filter != NULL && strstr(tests[i].name, filter) == NULL) {
            continue;
        }
        current_failures = 0;
        tests[i].fn();
        run++;
        if (current_failures != 0) {
            failed++;
            printf("FAIL %s\n", tests[i].name);
        } else {
            printf("PASS %s\n", tests[i].name);
        }
    }
    printf("smytest: %zu run, %zu passed, %zu failed\n", run, run - failed, failed);
    if (run == 0) {
        fprintf(stderr, "smytest: no tests ran\n");
        return 1;
    }
    return failed == 0 ? 0 : 1;
}
