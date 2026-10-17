/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

/* Loading descriptor tables (System V ABI: first argument in rdi). */

    .intel_syntax noprefix
    .section .text

/* void gdt_load(const struct descriptor_pointer *gdtr)
 * Loads the GDT, reloads the data segment registers with the kernel data
 * selector (0x10), clears FS/GS, and reloads CS (0x08) with a far return. */
    .global gdt_load
    .type gdt_load, @function
gdt_load:
    lgdt [rdi]
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    xor eax, eax
    mov fs, ax
    mov gs, ax
    pop rax                 /* our return address */
    push 0x08               /* new CS */
    push rax
    retfq
    .size gdt_load, . - gdt_load

/* void tss_load(uint16_t selector) */
    .global tss_load
    .type tss_load, @function
tss_load:
    ltr di
    ret
    .size tss_load, . - tss_load

/* void idt_load(const struct descriptor_pointer *idtr) */
    .global idt_load
    .type idt_load, @function
idt_load:
    lidt [rdi]
    ret
    .size idt_load, . - idt_load

    .section .note.GNU-stack, "", @progbits
