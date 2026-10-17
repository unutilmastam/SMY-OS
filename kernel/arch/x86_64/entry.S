/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

/*
 * Kernel entry point. Limine enters here in 64-bit long mode, interrupts
 * disabled, on a bootloader-provided stack (Limine protocol, x86-64 machine
 * state). We only make the stack 16-byte aligned as the System V ABI expects
 * at a call, end the frame-pointer chain for stack traces, and call kmain.
 */

    .intel_syntax noprefix
    .section .text
    .global _start
    .type _start, @function
_start:
    and rsp, -16
    xor ebp, ebp
    call kmain
    /* kmain does not return; if it ever does, stop this CPU for good. */
1:
    cli
    hlt
    jmp 1b
    .size _start, . - _start

    .section .note.GNU-stack, "", @progbits
