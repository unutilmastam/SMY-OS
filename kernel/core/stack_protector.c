/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

/*
 * Support for -fstack-protector-strong. On the x86_64-unknown-none-elf
 * target the compiler reads the canary from the global __stack_chk_guard
 * and calls __stack_chk_fail on a mismatch.
 *
 * The canary is a fixed value for now: there is no entropy source before
 * Phase 7 (timers; RDRAND/RDSEED detection). A fixed canary still catches
 * accidental overflows, but not an attacker who knows the value — see
 * THREAT_MODEL.md.
 */

#include "core/compiler.h"
#include "core/panic.h"

#include <stdint.h>

/*
 * The reserved names are fixed by the compiler's stack-protector ABI, so the
 * reserved-identifier checks are silenced for exactly these symbols.
 * Declared here to satisfy -Wmissing-prototypes; only compiler-generated
 * code uses them.
 */
// NOLINTBEGIN(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
extern uintptr_t __stack_chk_guard;
SMY_NORETURN void __stack_chk_fail(void);

uintptr_t __stack_chk_guard = 0x595e9fbd94fda766u;

void __stack_chk_fail(void)
{
    panic("stack smashing detected");
}
// NOLINTEND(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
