/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

#include "core/log.h"

#include "core/fmt.h"
#include "drivers/serial/uart16550.h"

#include <stddef.h>

static enum log_level min_level = LOG_INFO;

static void serial_sink(char c, void *ctx)
{
    (void)ctx;
    uart16550_putc(c);
}

bool log_init(void)
{
    return uart16550_init(UART16550_COM1);
}

void log_set_level(enum log_level level)
{
    min_level = level;
}

static const char *level_tag(enum log_level level)
{
    switch (level) {
    case LOG_DEBUG:
        return "[debug] ";
    case LOG_WARN:
        return "[warn] ";
    case LOG_ERROR:
        return "[error] ";
    case LOG_INFO:
    default:
        return "";
    }
}

void vklog(enum log_level level, const char *fmt, va_list args)
{
    if (level < min_level) {
        return;
    }
    log_write("SMY-OS: ");
    log_write(level_tag(level));
    fmt_vformat(serial_sink, NULL, fmt, args);
    uart16550_putc('\n');
}

void klog(enum log_level level, const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    vklog(level, fmt, args);
    va_end(args);
}

void log_write(const char *s)
{
    while (*s != '\0') {
        uart16550_putc(*s++);
    }
}

void log_vprintf(const char *fmt, va_list args)
{
    fmt_vformat(serial_sink, NULL, fmt, args);
}

void log_printf(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    log_vprintf(fmt, args);
    va_end(args);
}
