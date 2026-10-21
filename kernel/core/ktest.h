/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

/*
 * ktest: in-kernel self-tests, run in QEMU when the kernel command line
 * contains "smy.ktest=<selection>" (docs/kernel/ktest.md):
 *   all     every test except panic tests
 *   <name>  exactly that test (also panic tests)
 * Results go to the serial log; the run ends with arch_test_exit(), which
 * QEMU's isa-debug-exit turns into the process exit code.
 *
 * Tests are registered at link time in the .ktest section:
 *
 *   KTEST(cpu_catches_invalid_opcode) { KTEST_ASSERT(...); }
 *
 * KTEST_PANIC(name) declares a test that must end in a kernel panic; it is
 * run only when selected by name, and passes only via the panic path
 * (checked by the integration test, tests/integration/ktest.sh).
 *
 * KTEST_MANUAL(name) declares a test that needs something from outside the
 * guest (e.g. keystrokes injected through the QEMU monitor); it also runs
 * only when selected by name.
 */

#ifndef SMY_KERNEL_CORE_KTEST_H
#define SMY_KERNEL_CORE_KTEST_H

#include "core/compiler.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum ktest_kind {
    KTEST_KIND_NORMAL = 0,
    KTEST_KIND_PANIC,
    KTEST_KIND_MANUAL,
};

struct ktest {
    const char *name;
    void (*fn)(void);
    enum ktest_kind kind;
};

#define KTEST_DEFINE_(test_name, test_kind)                                                        \
    static void test_name(void);                                                                   \
    SMY_USED SMY_SECTION(".ktest") SMY_ALIGNED(8) static const struct ktest test_name##_ktest = {  \
        .name = #test_name, .fn = (test_name), .kind = (test_kind)};                               \
    static void test_name(void)

#define KTEST(test_name) KTEST_DEFINE_(test_name, KTEST_KIND_NORMAL)
#define KTEST_PANIC(test_name) KTEST_DEFINE_(test_name, KTEST_KIND_PANIC)
#define KTEST_MANUAL(test_name) KTEST_DEFINE_(test_name, KTEST_KIND_MANUAL)

void ktest_fail(const char *file, int line, const char *fmt, ...) SMY_PRINTF(3, 4);

#define KTEST_ASSERT(cond)                                                                         \
    do {                                                                                           \
        if (!(cond)) {                                                                             \
            ktest_fail(__FILE__, __LINE__, "%s", #cond);                                           \
        }                                                                                          \
    } while (0)

#define KTEST_ASSERT_EQ_U64(actual, expected)                                                      \
    do {                                                                                           \
        const uint64_t ktest_a_ = (uint64_t)(actual);                                              \
        const uint64_t ktest_e_ = (uint64_t)(expected);                                            \
        if (ktest_a_ != ktest_e_) {                                                                \
            ktest_fail(__FILE__, __LINE__, "%s == 0x%llx, expected 0x%llx", #actual,               \
                       (unsigned long long)ktest_a_, (unsigned long long)ktest_e_);                \
        }                                                                                          \
    } while (0)

/* Run the tests chosen by `selection` (not NUL-terminated) and exit. */
SMY_NORETURN void ktest_run(const char *selection, size_t selection_len);

#endif /* SMY_KERNEL_CORE_KTEST_H */
