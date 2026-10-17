/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

/*
 * x86-64 descriptor encodings: GDT segment and TSS descriptors, IDT gates
 * and the TSS layout. Pure functions, host-testable. Layouts follow the
 * Intel SDM Vol. 3A (§3.4.5, §6.14.1, §8.7) / AMD APM Vol. 2.
 */

#ifndef SMY_KERNEL_ARCH_X86_64_DESCRIPTORS_H
#define SMY_KERNEL_ARCH_X86_64_DESCRIPTORS_H

#include "core/compiler.h"

#include <stdint.h>

/* GDT layout (docs/kernel/cpu.md). User data comes before user code
 * because SYSRET (Phase 10) derives both selectors from one STAR field. */
enum {
    GDT_NULL = 0x00,
    GDT_KERNEL_CODE = 0x08,
    GDT_KERNEL_DATA = 0x10,
    GDT_USER_DATA = 0x18, /* used with RPL 3 */
    GDT_USER_CODE = 0x20, /* used with RPL 3 */
    GDT_TSS = 0x28,       /* 16-byte system descriptor: two slots */
    GDT_ENTRIES = 7,
};

/* Access bytes. */
enum {
    GDT_ACCESS_KERNEL_CODE = 0x9A, /* present, DPL0, code, execute/read */
    GDT_ACCESS_KERNEL_DATA = 0x92, /* present, DPL0, data, read/write */
    GDT_ACCESS_USER_CODE = 0xFA,   /* present, DPL3, code, execute/read */
    GDT_ACCESS_USER_DATA = 0xF2,   /* present, DPL3, data, read/write */
    GDT_ACCESS_TSS = 0x89,         /* present, DPL0, 64-bit TSS (available) */
};

/* Flags nibble: G (4 KiB granularity), D/B, L (64-bit code), AVL. */
enum {
    GDT_FLAGS_CODE64 = 0xA, /* G + L */
    GDT_FLAGS_DATA = 0xC,   /* G + D/B */
};

enum {
    IDT_GATE_INTERRUPT_DPL0 = 0x8E, /* present, DPL0, 64-bit interrupt gate */
    IDT_ENTRIES = 256,
};

/* Interrupt Stack Table slots (1-based, 0 = no stack switch). */
enum {
    IST_NONE = 0,
    IST_DOUBLE_FAULT = 1,
    IST_NMI = 2,
    IST_MACHINE_CHECK = 3,
};

struct idt_gate {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t reserved;
} SMY_PACKED;

struct tss64 {
    uint32_t reserved0;
    uint64_t rsp[3]; /* stacks for CPL 0..2 on privilege change */
    uint64_t reserved1;
    uint64_t ist[7]; /* IST1..IST7 */
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iopb_offset;
} SMY_PACKED;

struct descriptor_pointer {
    uint16_t limit;
    uint64_t base;
} SMY_PACKED;

uint64_t gdt_segment(uint32_t base, uint32_t limit, uint8_t access, uint8_t flags);
void gdt_tss_descriptor(uint64_t out[2], uint64_t base, uint32_t limit);
struct idt_gate idt_make_gate(uint64_t handler, uint16_t selector, uint8_t ist, uint8_t type_attr);
uint64_t idt_gate_handler(const struct idt_gate *gate);

#endif /* SMY_KERNEL_ARCH_X86_64_DESCRIPTORS_H */
