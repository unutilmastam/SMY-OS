/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

#include "arch/x86_64/apic.h"

#include "arch/x86_64/io.h"
#include "arch/x86_64/regs.h"
#include "memory/vmm.h"

#include <stddef.h>

/* ---------------------------------------------------------------- 8259 PIC */

enum {
    PIC1_CMD = 0x20,
    PIC1_DATA = 0x21,
    PIC2_CMD = 0xA0,
    PIC2_DATA = 0xA1,
};

void pic_disable(void)
{
    /* ICW1..ICW4: remap to 0xE0/0xE8 so a stray PIC interrupt can never
     * look like a CPU exception, then mask all 16 lines. */
    io_out8(PIC1_CMD, 0x11);
    io_out8(PIC2_CMD, 0x11);
    io_out8(PIC1_DATA, 0xE0);
    io_out8(PIC2_DATA, 0xE8);
    io_out8(PIC1_DATA, 4);
    io_out8(PIC2_DATA, 2);
    io_out8(PIC1_DATA, 1);
    io_out8(PIC2_DATA, 1);
    io_out8(PIC1_DATA, 0xFF);
    io_out8(PIC2_DATA, 0xFF);
}

/* -------------------------------------------------------------- local APIC */

enum {
    MSR_APIC_BASE = 0x1B,
    APIC_BASE_ENABLE = 1u << 11,
    APIC_BASE_X2APIC = 1u << 10,

    LAPIC_ID = 0x020,
    LAPIC_TPR = 0x080,
    LAPIC_EOI = 0x0B0,
    LAPIC_SVR = 0x0F0,
    LAPIC_ESR = 0x280,
    LAPIC_LVT_TIMER = 0x320,
    LAPIC_LVT_ERROR = 0x370,
    LAPIC_TIMER_INITIAL = 0x380,
    LAPIC_TIMER_CURRENT = 0x390,
    LAPIC_TIMER_DIVIDE = 0x3E0,

    LVT_MASKED = 1u << 16,
    LVT_PERIODIC = 1u << 17,
    SVR_ENABLE = 1u << 8,
    DIVIDE_BY_16 = 0x3,
};

static volatile uint32_t *lapic_mmio;
static bool x2apic;

static uint32_t lapic_read(uint32_t reg)
{
    return x2apic ? (uint32_t)read_msr(0x800u + (reg >> 4)) : lapic_mmio[reg / 4];
}

static void lapic_write(uint32_t reg, uint32_t value)
{
    if (x2apic) {
        write_msr(0x800u + (reg >> 4), value);
    } else {
        lapic_mmio[reg / 4] = value;
    }
}

void lapic_init(uint64_t phys)
{
    const uint64_t base = read_msr(MSR_APIC_BASE);

    x2apic = (base & APIC_BASE_X2APIC) != 0;
    if ((base & APIC_BASE_ENABLE) == 0) {
        write_msr(MSR_APIC_BASE, base | APIC_BASE_ENABLE);
    }
    if (!x2apic) {
        lapic_mmio = vmm_map_physical(phys, 4096, VM_WRITE | VM_UNCACHED);
    }
    lapic_write(LAPIC_TPR, 0);
    lapic_write(LAPIC_LVT_ERROR, VECTOR_LAPIC_ERROR);
    lapic_write(LAPIC_ESR, 0);
    lapic_write(LAPIC_LVT_TIMER, LVT_MASKED);
    lapic_write(LAPIC_SVR, SVR_ENABLE | VECTOR_SPURIOUS);
    lapic_eoi();
}

uint32_t lapic_id(void)
{
    const uint32_t id = lapic_read(LAPIC_ID);

    return x2apic ? id : id >> 24;
}

bool lapic_is_x2apic(void)
{
    return x2apic;
}

void lapic_eoi(void)
{
    lapic_write(LAPIC_EOI, 0);
}

uint32_t lapic_timer_measure(void (*wait)(void))
{
    lapic_write(LAPIC_TIMER_DIVIDE, DIVIDE_BY_16);
    lapic_write(LAPIC_LVT_TIMER, LVT_MASKED); /* one-shot, no interrupt */
    lapic_write(LAPIC_TIMER_INITIAL, 0xFFFFFFFFu);
    wait();
    {
        const uint32_t left = lapic_read(LAPIC_TIMER_CURRENT);

        lapic_write(LAPIC_TIMER_INITIAL, 0);
        return 0xFFFFFFFFu - left;
    }
}

void lapic_timer_start_periodic(uint32_t initial_count, uint8_t vector)
{
    lapic_write(LAPIC_TIMER_DIVIDE, DIVIDE_BY_16);
    lapic_write(LAPIC_LVT_TIMER, LVT_PERIODIC | vector);
    lapic_write(LAPIC_TIMER_INITIAL, initial_count);
}

/* ---------------------------------------------------------------- I/O APIC */

#define MAX_IOAPICS 8

struct ioapic {
    volatile uint32_t *mmio;
    uint32_t gsi_base;
    uint32_t pins;
};

static struct ioapic ioapics[MAX_IOAPICS];
static unsigned ioapic_total;

static uint32_t ioapic_read(const struct ioapic *io, uint32_t reg)
{
    io->mmio[0] = reg;
    return io->mmio[4]; /* IOWIN at +0x10 */
}

static void ioapic_write(const struct ioapic *io, uint32_t reg, uint32_t value)
{
    io->mmio[0] = reg;
    io->mmio[4] = value;
}

bool ioapic_init(uint64_t phys, uint32_t gsi_base)
{
    struct ioapic *io;

    if (ioapic_total == MAX_IOAPICS) {
        return false;
    }
    io = &ioapics[ioapic_total];
    io->mmio = vmm_map_physical(phys, 4096, VM_WRITE | VM_UNCACHED);
    io->gsi_base = gsi_base;
    io->pins = ((ioapic_read(io, 0x01) >> 16) & 0xFF) + 1;
    /* Mask every pin until a driver asks for it. */
    for (uint32_t pin = 0; pin < io->pins; pin++) {
        ioapic_write(io, 0x10 + 2 * pin, LVT_MASKED);
        ioapic_write(io, 0x11 + 2 * pin, 0);
    }
    ioapic_total++;
    return true;
}

unsigned ioapic_count(void)
{
    return ioapic_total;
}

bool ioapic_route(uint32_t gsi, uint8_t vector, uint16_t inti_flags, uint32_t dest_apic_id)
{
    for (unsigned i = 0; i < ioapic_total; i++) {
        const struct ioapic *io = &ioapics[i];

        if (gsi >= io->gsi_base && gsi < io->gsi_base + io->pins) {
            const uint32_t pin = gsi - io->gsi_base;
            uint32_t low = vector; /* fixed delivery, physical destination */

            if ((inti_flags & 0x3u) == 0x3u) {
                low |= 1u << 13; /* active low */
            }
            if ((inti_flags & 0xCu) == 0xCu) {
                low |= 1u << 15; /* level triggered */
            }
            ioapic_write(io, 0x11 + 2 * pin, dest_apic_id << 24);
            ioapic_write(io, 0x10 + 2 * pin, low);
            return true;
        }
    }
    return false;
}

/* -------------------------------------------------------------------- HPET */

static volatile uint64_t *hpet_mmio;
static uint64_t hpet_fs;

bool hpet_init(uint64_t phys)
{
    uint64_t caps;

    hpet_mmio = vmm_map_physical(phys, 4096, VM_WRITE | VM_UNCACHED);
    caps = hpet_mmio[0];
    hpet_fs = caps >> 32;
    /* Spec: period <= 100 ns; we need a 64-bit main counter (bit 13). */
    if (hpet_fs == 0 || hpet_fs > 100000000u || (caps & (1u << 13)) == 0) {
        hpet_mmio = NULL;
        return false;
    }
    hpet_mmio[0x10 / 8] = (hpet_mmio[0x10 / 8] & ~3ull) | 1u; /* enable, no legacy routing */
    return true;
}

uint64_t hpet_counter(void)
{
    return hpet_mmio != NULL ? hpet_mmio[0xF0 / 8] : 0;
}

uint64_t hpet_period_fs(void)
{
    return hpet_fs;
}

/* --------------------------------------------------------------------- PIT */

bool pit_wait_10ms(void)
{
    const uint16_t count = 11932;                                /* 1.193182 MHz * 10 ms */
    uint8_t port61 = (uint8_t)((io_in8(0x61) & ~0x02u) | 0x01u); /* gate on, speaker off */

    io_out8(0x61, port61);
    io_out8(0x43, 0xB0); /* channel 2, lobyte/hibyte, mode 0 */
    io_out8(0x42, (uint8_t)count);
    io_out8(0x42, (uint8_t)(count >> 8));
    /* Restart the count with a rising gate edge. */
    io_out8(0x61, (uint8_t)(port61 & ~0x01u));
    io_out8(0x61, port61);
    for (uint32_t i = 0; i < 100000000u; i++) {
        if ((io_in8(0x61) & 0x20u) != 0) {
            return true;
        }
    }
    return false; /* no PIT: the output never went high */
}
