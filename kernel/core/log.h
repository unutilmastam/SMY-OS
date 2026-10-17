/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

/*
 * Kernel log: formatted text to the serial console, one line per call.
 *
 * Line format (docs/kernel/logging.md):
 *   info   "SMY-OS: <message>"
 *   other  "SMY-OS: [debug|warn|error] <message>"
 * Boot tests match exact info lines such as "SMY-OS: boot OK", so keep their
 * wording stable. Messages below the current level are dropped.
 */

#ifndef SMY_KERNEL_CORE_LOG_H
#define SMY_KERNEL_CORE_LOG_H

#include "core/compiler.h"

#include <stdarg.h>
#include <stdbool.h>

enum log_level {
    LOG_DEBUG = 0,
    LOG_INFO = 1,
    LOG_WARN = 2,
    LOG_ERROR = 3,
};

/* Bring up the serial console; returns false if no serial port answered. */
bool log_init(void);

void log_set_level(enum log_level level);

/* Log one line (a trailing newline is added). */
void klog(enum log_level level, const char *fmt, ...) SMY_PRINTF(2, 3);
void vklog(enum log_level level, const char *fmt, va_list args);

/* Raw output without prefix or level filtering, for panic and test reports. */
void log_write(const char *s);
void log_printf(const char *fmt, ...) SMY_PRINTF(1, 2);
void log_vprintf(const char *fmt, va_list args);

#endif /* SMY_KERNEL_CORE_LOG_H */
