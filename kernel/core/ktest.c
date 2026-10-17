/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

#include "core/ktest.h"

#include "arch/cpu.h"
#include "core/cmdline.h"
#include "core/log.h"

#include <stdarg.h>

/* Bounds of the .ktest section, defined in the linker script. */
extern const struct ktest ktest_section_start[];
extern const struct ktest ktest_section_end[];

static unsigned current_failures;

void ktest_fail(const char *file, int line, const char *fmt, ...)
{
    va_list args;

    current_failures++;
    log_printf("SMY-OS: ktest:   %s:%d: ", file, line);
    va_start(args, fmt);
    log_vprintf(fmt, args);
    va_end(args);
    log_write("\n");
}

static bool selected(const struct ktest *t, const char *selection, size_t len)
{
    if (cmdline_value_is(selection, len, t->name)) {
        return true;
    }
    return !t->expects_panic && cmdline_value_is(selection, len, "all");
}

void ktest_run(const char *selection, size_t selection_len)
{
    unsigned run = 0;
    unsigned failed = 0;

    /* Count via addresses: comparing pointers to two different linker
     * symbols is undefined in C. */
    const size_t count =
        ((uintptr_t)ktest_section_end - (uintptr_t)ktest_section_start) / sizeof(struct ktest);

    log_printf("SMY-OS: ktest: selection '%.*s'\n", (int)selection_len, selection);
    for (size_t i = 0; i < count; i++) {
        const struct ktest *t = &ktest_section_start[i];

        if (!selected(t, selection, selection_len)) {
            continue;
        }
        current_failures = 0;
        log_printf("SMY-OS: ktest: RUN  %s\n", t->name);
        t->fn();
        run++;
        if (t->expects_panic) {
            /* Reaching this line means the expected panic did not happen. */
            log_printf("SMY-OS: ktest: FAIL %s (expected a panic)\n", t->name);
            failed++;
        } else if (current_failures != 0) {
            log_printf("SMY-OS: ktest: FAIL %s\n", t->name);
            failed++;
        } else {
            log_printf("SMY-OS: ktest: PASS %s\n", t->name);
        }
    }
    if (run == 0) {
        log_write("SMY-OS: ktest: no test matches the selection\n");
        arch_test_exit(false);
    }
    log_printf("SMY-OS: ktest: %u run, %u passed, %u failed\n", run, run - failed, failed);
    arch_test_exit(failed == 0);
}
