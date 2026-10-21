/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

/*
 * SMY Kernel link layout (x86-64). Higher-half kernel at the top 2 GiB
 * (-mcmodel=kernel), as the Limine protocol requires for non-relocatable
 * executables. Each segment gets only the permissions it needs, so no
 * segment is both writable and executable (W^X). The kernel_* symbols mark
 * page-aligned section bounds; memory/vmm.c maps each range with exactly
 * these rights in the kernel's own page tables.
 */

OUTPUT_FORMAT(elf64-x86-64)
ENTRY(_start)

PHDRS
{
    limine_requests PT_LOAD FLAGS(6);   /* R+W: bootloader writes responses */
    text            PT_LOAD FLAGS(5);   /* R+X */
    rodata          PT_LOAD FLAGS(4);   /* R */
    data            PT_LOAD FLAGS(6);   /* R+W */
}

SECTIONS
{
    . = 0xffffffff80000000;
    kernel_start = .;

    .limine_requests : {
        KEEP(*(.limine_requests_start))
        KEEP(*(.limine_requests))
        KEEP(*(.limine_requests_end))
    } :limine_requests

    . = ALIGN(CONSTANT(MAXPAGESIZE));
    kernel_text_start = .;
    .text : {
        *(.text .text.*)
    } :text
    kernel_text_end = .;

    . = ALIGN(CONSTANT(MAXPAGESIZE));
    kernel_rodata_start = .;
    .rodata : {
        *(.rodata .rodata.*)
        /* ktest registry (core/ktest.h), read-only */
        . = ALIGN(8);
        ktest_section_start = .;
        KEEP(*(.ktest))
        ktest_section_end = .;
    } :rodata
    kernel_rodata_end = .;

    . = ALIGN(CONSTANT(MAXPAGESIZE));
    kernel_data_start = .;
    .data : {
        *(.data .data.*)
    } :data

    .bss : {
        *(.bss .bss.*)
        *(COMMON)
    } :data
    . = ALIGN(CONSTANT(MAXPAGESIZE));
    kernel_end = .;

    /DISCARD/ : {
        *(.eh_frame*)
        *(.note .note.*)
    }
}
