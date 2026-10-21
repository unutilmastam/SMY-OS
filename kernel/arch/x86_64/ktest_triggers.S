/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

/*
 * Instructions that raise specific CPU exceptions, for ktests. Each
 * trigger has a *_resume label right after the faulting instruction; a test
 * arms the exception handler with that address so execution continues there
 * (exceptions_expect() in exceptions.h).
 */

    .intel_syntax noprefix
    .section .text

/* void ktest_trigger_ud(void): #UD (vector 6) */
    .global ktest_trigger_ud, ktest_trigger_ud_resume
ktest_trigger_ud:
    ud2
ktest_trigger_ud_resume:
    ret

/* void ktest_trigger_bp(void): #BP (vector 3, a trap: RIP already points
 * past int3, which is ktest_trigger_bp_resume) */
    .global ktest_trigger_bp, ktest_trigger_bp_resume
ktest_trigger_bp:
    int3
ktest_trigger_bp_resume:
    ret

/* uint64_t ktest_trigger_load(uint64_t address): reads 8 bytes from
 * `address`. Unmapped address -> #PF (14); non-canonical -> #GP (13). */
    .global ktest_trigger_load, ktest_trigger_load_resume
ktest_trigger_load:
    mov rax, [rdi]
ktest_trigger_load_resume:
    ret

/* void ktest_trigger_de(void): unsigned divide by zero -> #DE (vector 0) */
    .global ktest_trigger_de, ktest_trigger_de_resume
ktest_trigger_de:
    xor edx, edx
    mov eax, 1
    xor ecx, ecx
    div ecx
ktest_trigger_de_resume:
    ret

/* void ktest_trigger_store(uint64_t address, uint64_t value): writes 8
 * bytes. Read-only page -> #PF (protection violation, write). */
    .global ktest_trigger_store, ktest_trigger_store_resume
ktest_trigger_store:
    mov [rdi], rsi
ktest_trigger_store_resume:
    ret

/* void ktest_trigger_call(uint64_t address): calls code at `address`.
 * No-execute page -> #PF (instruction fetch) with RIP = address and the
 * return address already pushed; resuming at the label runs `ret` twice
 * (first to the label itself, then to the caller), which unwinds both. */
    .global ktest_trigger_call, ktest_trigger_call_resume
ktest_trigger_call:
    call rdi
ktest_trigger_call_resume:
    ret

    .section .note.GNU-stack, "", @progbits
