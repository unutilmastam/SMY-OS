/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

/*
 * x86-64 platform bring-up (arch/platform.h, docs/kernel/interrupts.md):
 * ACPI MADT -> mask the 8259 PICs, local APIC, I/O APICs; HPET (or PIT) to
 * calibrate the local APIC timer; interrupt dispatch for vectors 32-255.
 */

#include "arch/platform.h"

#include "acpi/acpi.h"
#include "arch/x86_64/apic.h"
#include "arch/x86_64/exceptions.h"
#include "core/log.h"
#include "core/panic.h"
#include "core/time.h"
#include "memory/vmm.h"

#include <stddef.h>

#define ISA_IRQS 16

struct isa_handler {
    arch_irq_handler fn;
    void *ctx;
    volatile uint64_t count;
};

static struct acpi acpi;
static struct acpi_madt_info madt;
static struct isa_handler isa_handlers[ISA_IRQS];
static volatile uint64_t ticks;
static bool have_hpet;
static uint32_t bsp_apic_id;
static volatile uint64_t spurious_count;

static const void *acpi_map(void *ctx, uint64_t phys, size_t len)
{
    (void)ctx;
    return vmm_map_physical(phys, len, VM_READ);
}

static const struct acpi_mem_ops acpi_ops = {.map = acpi_map, .ctx = NULL};

static void wait_10ms_hpet(void)
{
    const uint64_t end = hpet_counter() + time_ns_to_counter(10 * NS_PER_MS, hpet_period_fs());

    while (hpet_counter() < end) {
        __asm__ volatile("pause");
    }
}

static void wait_10ms_pit(void)
{
    if (!pit_wait_10ms()) {
        panic("platform: neither HPET nor PIT available to calibrate the timer");
    }
}

/* Called by exception_dispatch for vectors 32-255. */
void irq_dispatch(uint64_t vector);

void irq_dispatch(uint64_t vector)
{
    if (vector == VECTOR_SPURIOUS) {
        spurious_count++;
        return; /* no EOI for spurious interrupts */
    }
    if (vector == VECTOR_TIMER) {
        ticks++;
    } else if (vector >= VECTOR_ISA_BASE && vector < VECTOR_ISA_BASE + ISA_IRQS) {
        struct isa_handler *h = &isa_handlers[vector - VECTOR_ISA_BASE];

        h->count++;
        if (h->fn != NULL) {
            h->fn(h->ctx);
        }
    } else if (vector == VECTOR_LAPIC_ERROR) {
        klog(LOG_WARN, "platform: local APIC error interrupt");
    } else {
        klog(LOG_WARN, "platform: unexpected interrupt vector %llu", (unsigned long long)vector);
    }
    lapic_eoi();
}

void arch_platform_init(uint64_t rsdp_phys)
{
    enum acpi_status st;
    const struct acpi_sdt_header *hpet_table;
    struct acpi_hpet_info hpet_info;
    uint32_t per_10ms;

    st = acpi_init(&acpi, &acpi_ops, rsdp_phys);
    if (st != ACPI_OK) {
        panic("platform: ACPI: %s", acpi_status_name(st));
    }
    st = acpi_parse_madt(acpi_find_table(&acpi, "APIC", 0), &madt);
    if (st != ACPI_OK) {
        panic("platform: ACPI MADT: %s", acpi_status_name(st));
    }

    /* The 8259s are masked even if the MADT says there are none: harmless. */
    pic_disable();
    lapic_init(madt.lapic_address);
    bsp_apic_id = lapic_id();
    for (unsigned i = 0; i < madt.ioapic_count; i++) {
        if (!ioapic_init(madt.ioapics[i].address, madt.ioapics[i].gsi_base)) {
            panic("platform: too many I/O APICs");
        }
    }

    hpet_table = acpi_find_table(&acpi, "HPET", 0);
    have_hpet = hpet_table != NULL && acpi_parse_hpet(hpet_table, &hpet_info) == ACPI_OK &&
                hpet_init(hpet_info.address);

    per_10ms = lapic_timer_measure(have_hpet ? wait_10ms_hpet : wait_10ms_pit);
    if (per_10ms < 100) {
        panic("platform: local APIC timer did not run during calibration");
    }
    lapic_timer_start_periodic(per_10ms / (ARCH_TIMER_HZ / 100), VECTOR_TIMER);

    klog(LOG_INFO,
         "platform: ACPI %s (rev %u), %u CPU(s), %u I/O APIC(s), local APIC %s id %u; timer %u Hz "
         "calibrated with %s (%u counts/10 ms)",
         acpi.oem_id, acpi.revision, madt.cpu_count, ioapic_count(),
         lapic_is_x2apic() ? "x2APIC" : "xAPIC", bsp_apic_id, ARCH_TIMER_HZ,
         have_hpet ? "HPET" : "PIT", per_10ms);
}

uint64_t arch_clock_counter(void)
{
    return have_hpet ? hpet_counter() : ticks;
}

uint64_t arch_clock_period_fs(void)
{
    return have_hpet ? hpet_period_fs() : NS_PER_S / ARCH_TIMER_HZ * FS_PER_NS;
}

uint64_t arch_timer_ticks(void)
{
    return ticks;
}

bool arch_irq_attach_isa(unsigned irq, arch_irq_handler handler, void *ctx)
{
    uint16_t flags;
    uint32_t gsi;

    if (irq >= ISA_IRQS) {
        return false;
    }
    gsi = acpi_isa_irq_to_gsi(&madt, (uint8_t)irq, &flags);
    isa_handlers[irq].ctx = ctx;
    isa_handlers[irq].fn = handler;
    return ioapic_route(gsi, (uint8_t)(VECTOR_ISA_BASE + irq), flags, bsp_apic_id);
}

uint64_t arch_irq_count_isa(unsigned irq)
{
    return irq < ISA_IRQS ? isa_handlers[irq].count : 0;
}

void arch_interrupts_enable(void)
{
    __asm__ volatile("sti" : : : "memory");
}

void arch_interrupts_disable(void)
{
    __asm__ volatile("cli" : : : "memory");
}

void arch_wait_for_interrupt(void)
{
    /* sti takes effect after the next instruction, so no interrupt can slip
     * in between and leave hlt waiting for nothing. */
    __asm__ volatile("sti; hlt" : : : "memory");
}
