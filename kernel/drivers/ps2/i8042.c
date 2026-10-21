/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

#include "drivers/ps2/i8042.h"

#include "arch/platform.h"
#include "arch/x86_64/io.h"
#include "core/log.h"

#include <stddef.h>

enum {
    PORT_DATA = 0x60,
    PORT_STATUS = 0x64,  /* read */
    PORT_COMMAND = 0x64, /* write */

    STATUS_OUTPUT_FULL = 1u << 0,
    STATUS_INPUT_FULL = 1u << 1,

    CMD_READ_CONFIG = 0x20,
    CMD_WRITE_CONFIG = 0x60,
    CMD_DISABLE_PORT2 = 0xA7,
    CMD_SELF_TEST = 0xAA,
    CMD_TEST_PORT1 = 0xAB,
    CMD_DISABLE_PORT1 = 0xAD,
    CMD_ENABLE_PORT1 = 0xAE,

    CONFIG_PORT1_IRQ = 1u << 0,
    CONFIG_PORT2_IRQ = 1u << 1,
    CONFIG_PORT1_CLOCK_OFF = 1u << 4,
    CONFIG_TRANSLATION = 1u << 6,

    WAIT_LIMIT = 200000, /* polls; far longer than any real controller needs */
};

static struct kbd_set1 decoder;
static struct key_ring ring;

static bool wait_input_empty(void)
{
    for (int i = 0; i < WAIT_LIMIT; i++) {
        if ((io_in8(PORT_STATUS) & STATUS_INPUT_FULL) == 0) {
            return true;
        }
        cpu_pause();
    }
    return false;
}

static bool read_data(uint8_t *out)
{
    for (int i = 0; i < WAIT_LIMIT; i++) {
        if ((io_in8(PORT_STATUS) & STATUS_OUTPUT_FULL) != 0) {
            *out = io_in8(PORT_DATA);
            return true;
        }
        cpu_pause();
    }
    return false;
}

static bool command(uint8_t cmd)
{
    if (!wait_input_empty()) {
        return false;
    }
    io_out8(PORT_COMMAND, cmd);
    return true;
}

static bool write_config(uint8_t config)
{
    if (!command(CMD_WRITE_CONFIG) || !wait_input_empty()) {
        return false;
    }
    io_out8(PORT_DATA, config);
    return true;
}

static void flush_output(void)
{
    for (int i = 0; i < 64 && (io_in8(PORT_STATUS) & STATUS_OUTPUT_FULL) != 0; i++) {
        (void)io_in8(PORT_DATA);
    }
}

static void keyboard_irq(void *ctx)
{
    (void)ctx;
    /* One interrupt per byte; read everything that is waiting. */
    while ((io_in8(PORT_STATUS) & STATUS_OUTPUT_FULL) != 0) {
        struct key_event ev;

        if (kbd_set1_feed(&decoder, io_in8(PORT_DATA), &ev)) {
            (void)key_ring_push(&ring, ev);
        }
    }
}

static bool fail(const char *why)
{
    klog(LOG_WARN, "keyboard: no PS/2 keyboard (%s)", why);
    return false;
}

bool ps2_keyboard_init(void)
{
    uint8_t config;
    uint8_t reply;

    kbd_set1_init(&decoder);
    key_ring_init(&ring);

    /* A missing controller reads as 0xFF everywhere. */
    if (io_in8(PORT_STATUS) == 0xFF) {
        return fail("no controller");
    }
    if (!command(CMD_DISABLE_PORT1) || !command(CMD_DISABLE_PORT2)) {
        return fail("controller does not accept commands");
    }
    flush_output();
    if (!command(CMD_READ_CONFIG) || !read_data(&config)) {
        return fail("cannot read the configuration");
    }
    config &= (uint8_t) ~(CONFIG_PORT1_IRQ | CONFIG_PORT2_IRQ);
    if (!write_config(config)) {
        return fail("cannot write the configuration");
    }
    if (!command(CMD_SELF_TEST) || !read_data(&reply) || reply != 0x55) {
        return fail("controller self-test failed");
    }
    /* Some controllers reset their configuration during the self-test. */
    if (!command(CMD_TEST_PORT1) || !read_data(&reply) || reply != 0x00) {
        return fail("port 1 test failed");
    }
    config = (uint8_t)((config | CONFIG_PORT1_IRQ | CONFIG_TRANSLATION) & ~CONFIG_PORT1_CLOCK_OFF);
    if (!write_config(config) || !command(CMD_ENABLE_PORT1)) {
        return fail("cannot enable port 1");
    }
    flush_output();
    if (!arch_irq_attach_isa(1, keyboard_irq, NULL)) {
        return fail("IRQ 1 cannot be routed");
    }
    klog(LOG_INFO, "keyboard: PS/2 keyboard on IRQ 1 (scancode set 1 via translation)");
    return true;
}

bool ps2_keyboard_read(struct key_event *out)
{
    return key_ring_pop(&ring, out);
}

uint32_t ps2_keyboard_dropped(void)
{
    return ring.dropped;
}
