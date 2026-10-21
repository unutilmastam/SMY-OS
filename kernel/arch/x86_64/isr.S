/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

/*
 * Interrupt entry stubs for all 256 vectors: CPU exceptions (0-31) and
 * device/timer interrupts (32-255, which never have an error code).
 *
 * Every stub leaves the same layout on the stack (lowest address first) —
 * struct interrupt_frame in arch/x86_64/exceptions.h:
 *   r15 ... r8, rbp, rdi, rsi, rdx, rcx, rbx, rax,   (pushed by isr_common)
 *   vector, error_code,                              (pushed by the stub)
 *   rip, cs, rflags, rsp, ss                         (pushed by the CPU)
 * Vectors without a CPU error code push 0 so the layout is uniform.
 *
 * Alignment: the CPU aligns RSP to 16 before pushing its 5 qwords; error
 * code + vector + 15 registers make 22 qwords in total, so RSP is 16-byte
 * aligned at the call, as the System V ABI requires.
 */

    .intel_syntax noprefix
    .section .text

.macro ISR_NO_ERROR vector
    .type isr_stub_\vector, @function
isr_stub_\vector:
    push 0
    push \vector
    jmp isr_common
.endm

.macro ISR_ERROR vector
    .type isr_stub_\vector, @function
isr_stub_\vector:
    push \vector
    jmp isr_common
.endm

/* Vectors with an error code: 8, 10-14, 17, 21, 29, 30 (Intel SDM 3A §6.3). */
ISR_NO_ERROR 0
ISR_NO_ERROR 1
ISR_NO_ERROR 2
ISR_NO_ERROR 3
ISR_NO_ERROR 4
ISR_NO_ERROR 5
ISR_NO_ERROR 6
ISR_NO_ERROR 7
ISR_ERROR    8
ISR_NO_ERROR 9
ISR_ERROR    10
ISR_ERROR    11
ISR_ERROR    12
ISR_ERROR    13
ISR_ERROR    14
ISR_NO_ERROR 15
ISR_NO_ERROR 16
ISR_ERROR    17
ISR_NO_ERROR 18
ISR_NO_ERROR 19
ISR_NO_ERROR 20
ISR_ERROR    21
ISR_NO_ERROR 22
ISR_NO_ERROR 23
ISR_NO_ERROR 24
ISR_NO_ERROR 25
ISR_NO_ERROR 26
ISR_NO_ERROR 27
ISR_NO_ERROR 28
ISR_ERROR    29
ISR_ERROR    30
ISR_NO_ERROR 31

/* Vectors 32-255: interrupts (no error code). */
.altmacro
.set vec, 32
.rept 224
    ISR_NO_ERROR %vec
    .set vec, vec + 1
.endr
.noaltmacro

    .type isr_common, @function
isr_common:
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    cld
    mov rdi, rsp            /* struct interrupt_frame * */
    call exception_dispatch
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    add rsp, 16             /* vector and error code */
    iretq
    .size isr_common, . - isr_common

/* Table of the 256 stub addresses, read by idt.c. */
.macro STUB_ADDRESS vector
    .quad isr_stub_\vector
.endm
    .section .rodata
    .balign 8
    .global isr_stub_table
isr_stub_table:
.altmacro
.set vec, 0
.rept 256
    STUB_ADDRESS %vec
    .set vec, vec + 1
.endr
.noaltmacro

    .section .note.GNU-stack, "", @progbits
