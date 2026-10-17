/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

/* ktests for architecture-independent core code that needs the real kernel
 * environment (portable logic is covered by host unit tests). */

#include "arch/cpu.h"
#include "core/fmt.h"
#include "core/ktest.h"
#include "core/panic.h"
#include "core/stacktrace.h"

/* Linker-script symbols around the kernel's code. */
extern const char kernel_text_start[];
extern const char kernel_text_end[];

static size_t walk_from_here(uintptr_t *frames, size_t max);

/* noinline keeps three real frames on the stack for the walker. */
__attribute__((noinline)) static size_t level3(uintptr_t *frames, size_t max)
{
    const size_t n = walk_from_here(frames, max);

    __asm__ volatile("" ::: "memory");
    return n;
}

/* The empty asm after each call keeps the compiler from turning the call
 * into a tail jump, which would remove the caller's frame. */
__attribute__((noinline)) static size_t level2(uintptr_t *frames, size_t max)
{
    const size_t n = level3(frames, max);

    __asm__ volatile("" ::: "memory");
    return n;
}

__attribute__((noinline)) static size_t level1(uintptr_t *frames, size_t max)
{
    const size_t n = level2(frames, max);

    __asm__ volatile("" ::: "memory");
    return n;
}

__attribute__((noinline)) static size_t walk_from_here(uintptr_t *frames, size_t max)
{
    const uintptr_t fp = (uintptr_t)__builtin_frame_address(0);
    uintptr_t lo;
    uintptr_t hi;

    if (!arch_stack_bounds(fp, &lo, &hi)) {
        return 0;
    }
    return stacktrace_walk(fp, lo, hi, frames, max);
}

KTEST(core_stack_trace_walks_kernel_frames)
{
    uintptr_t frames[16];
    const size_t n = level1(frames, 16);

    /* walk_from_here <- level3 <- level2 <- level1 <- this test <- runner ... */
    KTEST_ASSERT(n >= 4);
    for (size_t i = 0; i < n && i < 4; i++) {
        KTEST_ASSERT(frames[i] >= (uintptr_t)kernel_text_start);
        KTEST_ASSERT(frames[i] < (uintptr_t)kernel_text_end);
    }
}

KTEST(core_formatting_works_in_kernel)
{
    char buf[48];

    /* Same code as the host tests, built with kernel flags (no SSE, red
     * zone off, -mcmodel=kernel): check that va_list handling works. */
    fmt_snprintf(buf, sizeof buf, "%s %d %x %llu %p", "ok", -5, 0xbeefu, 123456789012ull,
                 (void *)0xffffffff80001000u);
    KTEST_ASSERT(buf[0] == 'o' && buf[1] == 'k');
    KTEST_ASSERT(fmt_snprintf(buf, sizeof buf, "%d", -2147483647 - 1) == 11);
}

KTEST_PANIC(panic_from_kernel_code)
{
    panic("ktest: deliberate panic with value %d", 42);
}
