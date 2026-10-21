/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

/* ktests for interrupts, timers and the PS/2 keyboard (Phase 7,
 * docs/kernel/interrupts.md, docs/kernel/input.md). */

#include "arch/platform.h"
#include "arch/x86_64/apic.h"
#include "arch/x86_64/idt.h"
#include "arch/x86_64/io.h"
#include "arch/x86_64/regs.h"
#include "core/ktest.h"
#include "core/log.h"
#include "core/time.h"
#include "drivers/ps2/i8042.h"

#define RATE_WINDOW_MS 200u

static bool interrupts_on(void)
{
    uint64_t flags;

    __asm__ volatile("pushfq; pop %0" : "=r"(flags));
    return (flags & (1u << 9)) != 0;
}

KTEST(platform_interrupt_controllers_are_set_up)
{
    KTEST_ASSERT(interrupts_on());
    /* Both 8259s fully masked. */
    KTEST_ASSERT_EQ_U64(io_in8(0x21), 0xFF);
    KTEST_ASSERT_EQ_U64(io_in8(0xA1), 0xFF);
    /* Local APIC enabled in the APIC base MSR; at least one I/O APIC. */
    KTEST_ASSERT((read_msr(0x1B) & (1u << 11)) != 0);
    KTEST_ASSERT(ioapic_count() >= 1);
    /* Every vector has a gate, including timer, ISA and spurious. */
    KTEST_ASSERT(idt_gate_handler(idt_gate(VECTOR_TIMER)) != 0);
    KTEST_ASSERT(idt_gate_handler(idt_gate(VECTOR_ISA_BASE + 1)) != 0);
    KTEST_ASSERT(idt_gate_handler(idt_gate(VECTOR_SPURIOUS)) != 0);
}

KTEST(platform_clock_is_monotonic_and_advances)
{
    uint64_t prev = clock_monotonic_ns();
    const uint64_t start = prev;

    for (int i = 0; i < 20000; i++) {
        const uint64_t now = clock_monotonic_ns();

        if (now < prev) {
            ktest_fail(__FILE__, __LINE__, "clock went back: %llu -> %llu",
                       (unsigned long long)prev, (unsigned long long)now);
            return;
        }
        prev = now;
    }
    clock_sleep_ns(5 * NS_PER_MS);
    KTEST_ASSERT(clock_monotonic_ns() - start >= 5 * NS_PER_MS);
}

/* Timer interrupts over a window measured by the monotonic clock (HPET when
 * present) must match ARCH_TIMER_HZ within 20 %. */
KTEST(platform_timer_ticks_at_configured_rate_by_clock)
{
    const uint64_t t0 = clock_monotonic_ns();
    const uint64_t k0 = clock_ticks();
    uint64_t ticks;

    clock_sleep_ns(RATE_WINDOW_MS * NS_PER_MS);
    ticks = clock_ticks() - k0;
    log_printf("SMY-OS: ktest:   %llu ticks in %llu ms (clock)\n", (unsigned long long)ticks,
               (unsigned long long)((clock_monotonic_ns() - t0) / NS_PER_MS));
    KTEST_ASSERT(ticks >= RATE_WINDOW_MS * ARCH_TIMER_HZ / 1000 * 8 / 10);
    KTEST_ASSERT(ticks <= RATE_WINDOW_MS * ARCH_TIMER_HZ / 1000 * 12 / 10);
}

/* The same against an independent reference: the PIT, polled. */
KTEST(platform_timer_ticks_at_configured_rate_by_pit)
{
    const uint64_t k0 = clock_ticks();
    uint64_t ticks;

    for (unsigned i = 0; i < RATE_WINDOW_MS / 10; i++) {
        if (!pit_wait_10ms()) {
            ktest_fail(__FILE__, __LINE__, "PIT did not count");
            return;
        }
    }
    ticks = clock_ticks() - k0;
    log_printf("SMY-OS: ktest:   %llu ticks in %u ms (PIT)\n", (unsigned long long)ticks,
               RATE_WINDOW_MS);
    KTEST_ASSERT(ticks >= RATE_WINDOW_MS * ARCH_TIMER_HZ / 1000 * 8 / 10);
    KTEST_ASSERT(ticks <= RATE_WINDOW_MS * ARCH_TIMER_HZ / 1000 * 12 / 10);
}

/*
 * Keystrokes injected through the QEMU monitor ("sendkey s", "m", "y",
 * "up", "ret": tests/integration/integration.mk) must arrive as these HID
 * key events, in order, each pressed and released.
 */
KTEST_MANUAL(keyboard_receives_injected_keys)
{
    static const uint16_t expected[] = {KEY_S, KEY_M, KEY_Y, KEY_UP, KEY_ENTER};
    const size_t total = 2 * (sizeof expected / sizeof expected[0]);
    const uint64_t deadline = clock_monotonic_ns() + 20 * NS_PER_S;
    const uint64_t irqs_before = arch_irq_count_isa(1);
    size_t got = 0;

    log_write("SMY-OS: ktest: waiting for keyboard input\n");
    while (got < total && clock_monotonic_ns() < deadline) {
        struct key_event ev;

        if (!ps2_keyboard_read(&ev)) {
            arch_wait_for_interrupt();
            continue;
        }
        log_printf("SMY-OS: ktest:   key 0x%02x %s\n", ev.key, ev.pressed ? "down" : "up");
        if (ev.key != expected[got / 2] || ev.pressed != (got % 2 == 0)) {
            ktest_fail(__FILE__, __LINE__, "event %zu: key 0x%02x %s, expected 0x%02x %s", got,
                       ev.key, ev.pressed ? "down" : "up", expected[got / 2],
                       got % 2 == 0 ? "down" : "up");
            return;
        }
        got++;
    }
    KTEST_ASSERT_EQ_U64(got, total);
    KTEST_ASSERT(arch_irq_count_isa(1) > irqs_before);
    KTEST_ASSERT_EQ_U64(ps2_keyboard_dropped(), 0);
}
