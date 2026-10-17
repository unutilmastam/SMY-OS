/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

#include "core/log.h"

#include "core/fmt.h"
#include "drivers/serial/uart16550.h"

bool log_init(void)
{
    return uart16550_init(UART16550_COM1);
}

void log_write(const char *s)
{
    while (*s != '\0') {
        uart16550_putc(*s++);
    }
}

void log_write_dec(uint64_t value)
{
    char buf[FMT_U64_BUF_SIZE];

    fmt_u64(buf, sizeof buf, value, 10);
    log_write(buf);
}

void log_write_hex(uint64_t value)
{
    char buf[FMT_U64_BUF_SIZE];

    fmt_u64(buf, sizeof buf, value, 16);
    log_write("0x");
    log_write(buf);
}
