/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

/* Host tests for kernel/core/stacktrace.c on a fake stack in memory. */

#include "core/stacktrace.h"
#include "harness/smytest.h"

#include <stdint.h>

#define WORDS 64

static uintptr_t stack[WORDS];

static uintptr_t at(size_t index)
{
    return (uintptr_t)&stack[index];
}

/* Frame at word `index`: [index] = next frame pointer, [index + 1] = return address. */
static void frame(size_t index, uintptr_t next_fp, uintptr_t ret)
{
    stack[index] = next_fp;
    stack[index + 1] = ret;
}

SMY_TEST(stacktrace_follows_a_valid_chain_until_zero)
{
    uintptr_t out[8];

    frame(4, at(10), 0x1111);
    frame(10, at(20), 0x2222);
    frame(20, 0, 0x3333);
    SMY_EXPECT_EQ_U64(stacktrace_walk(at(4), at(0), at(WORDS), out, 8), 3);
    SMY_EXPECT_EQ_U64(out[0], 0x1111);
    SMY_EXPECT_EQ_U64(out[1], 0x2222);
    SMY_EXPECT_EQ_U64(out[2], 0x3333);
}

SMY_TEST(stacktrace_respects_max)
{
    uintptr_t out[2];

    frame(4, at(10), 0x1111);
    frame(10, at(20), 0x2222);
    frame(20, 0, 0x3333);
    SMY_EXPECT_EQ_U64(stacktrace_walk(at(4), at(0), at(WORDS), out, 2), 2);
}

SMY_TEST(stacktrace_stops_on_loops_and_downward_links)
{
    uintptr_t out[8];

    frame(10, at(10), 0x1111); /* points to itself */
    SMY_EXPECT_EQ_U64(stacktrace_walk(at(10), at(0), at(WORDS), out, 8), 1);
    frame(10, at(4), 0x1111); /* points down the stack */
    frame(4, at(10), 0x2222);
    SMY_EXPECT_EQ_U64(stacktrace_walk(at(10), at(0), at(WORDS), out, 8), 1);
}

SMY_TEST(stacktrace_rejects_frames_outside_bounds_or_misaligned)
{
    uintptr_t out[8];

    frame(4, at(10), 0x1111);
    frame(10, at(20), 0x2222);
    /* Bounds end inside the second frame: only the first is followed. */
    SMY_EXPECT_EQ_U64(stacktrace_walk(at(4), at(0), at(11), out, 8), 1);
    /* Start below the bounds. */
    SMY_EXPECT_EQ_U64(stacktrace_walk(at(4), at(5), at(WORDS), out, 8), 0);
    /* Misaligned frame pointer. */
    SMY_EXPECT_EQ_U64(stacktrace_walk(at(4) + 1, at(0), at(WORDS), out, 8), 0);
    /* Null frame pointer and zero return address end the walk. */
    SMY_EXPECT_EQ_U64(stacktrace_walk(0, 0, UINTPTR_MAX, out, 8), 0);
    frame(30, at(40), 0);
    SMY_EXPECT_EQ_U64(stacktrace_walk(at(30), at(0), at(WORDS), out, 8), 0);
    /* Tiny bounds must not underflow. */
    SMY_EXPECT_EQ_U64(stacktrace_walk(8, 0, 8, out, 8), 0);
}
