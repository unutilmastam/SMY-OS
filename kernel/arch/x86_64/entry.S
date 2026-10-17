/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

/*
 * Kernel entry point. Limine enters here in 64-bit long mode, interrupts
 * disabled, on a bootloader-provided stack (Limine protocol, x86-64 machine
 * state). We switch to the kernel's own boot stack (known bounds, so stack
 * traces can be checked against it; a guard page follows in Phase 5), end
 * the frame-pointer chain and call kmain.
 */

    .intel_syntax noprefix

    .section .bss
    .balign 16
    .global kernel_boot_stack_bottom, kernel_boot_stack_top
kernel_boot_stack_bottom:
    .skip 65536
kernel_boot_stack_top:

    .section .text
    .global _start
    .type _start, @function
_start:
    lea rsp, [rip + kernel_boot_stack_top]
    xor ebp, ebp
    call kmain
    /* kmain does not return; if it ever does, stop this CPU for good. */
1:
    cli
    hlt
    jmp 1b
    .size _start, . - _start

    .section .note.GNU-stack, "", @progbits
