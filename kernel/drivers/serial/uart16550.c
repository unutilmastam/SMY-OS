/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

/* Register layout and bits: PC16550D UART datasheet (public). */

#include "drivers/serial/uart16550.h"

#include "arch/x86_64/io.h"

enum {
    REG_DATA = 0,        /* THR/RBR; divisor low byte when DLAB = 1 */
    REG_INT_ENABLE = 1,  /* IER; divisor high byte when DLAB = 1 */
    REG_FIFO_CTRL = 2,   /* FCR */
    REG_LINE_CTRL = 3,   /* LCR */
    REG_MODEM_CTRL = 4,  /* MCR */
    REG_LINE_STATUS = 5, /* LSR */
};

enum {
    LCR_8N1 = 0x03,
    LCR_DLAB = 0x80,
    FCR_ENABLE_CLEAR_14 = 0xC7, /* enable + clear FIFOs, 14-byte threshold */
    MCR_LOOPBACK_TEST = 0x1E,   /* loopback, RTS, OUT1, OUT2 */
    MCR_NORMAL = 0x0F,          /* DTR, RTS, OUT1, OUT2 */
    LSR_THR_EMPTY = 0x20,
};

/* Bounded wait so a stuck UART can never hang the kernel. */
#define TX_SPIN_LIMIT 100000u

static uint16_t uart_base;
static bool uart_ready;

bool uart16550_init(uint16_t base)
{
    const uint8_t probe = 0xAE;

    uart_base = base;
    uart_ready = false;

    io_out8((uint16_t)(base + REG_INT_ENABLE), 0x00);
    io_out8((uint16_t)(base + REG_LINE_CTRL), LCR_DLAB);
    io_out8((uint16_t)(base + REG_DATA), 0x01); /* divisor 1 = 115200 baud */
    io_out8((uint16_t)(base + REG_INT_ENABLE), 0x00);
    io_out8((uint16_t)(base + REG_LINE_CTRL), LCR_8N1);
    io_out8((uint16_t)(base + REG_FIFO_CTRL), FCR_ENABLE_CLEAR_14);

    io_out8((uint16_t)(base + REG_MODEM_CTRL), MCR_LOOPBACK_TEST);
    io_out8((uint16_t)(base + REG_DATA), probe);
    if (io_in8((uint16_t)(base + REG_DATA)) != probe) {
        return false;
    }

    io_out8((uint16_t)(base + REG_MODEM_CTRL), MCR_NORMAL);
    uart_ready = true;
    return true;
}

static void uart_write_byte(uint8_t byte)
{
    for (unsigned spin = 0; spin < TX_SPIN_LIMIT; spin++) {
        if ((io_in8((uint16_t)(uart_base + REG_LINE_STATUS)) & LSR_THR_EMPTY) != 0) {
            io_out8((uint16_t)(uart_base + REG_DATA), byte);
            return;
        }
        cpu_pause();
    }
}

void uart16550_putc(char c)
{
    if (!uart_ready) {
        return;
    }
    if (c == '\n') {
        uart_write_byte('\r');
    }
    uart_write_byte((uint8_t)c);
}
