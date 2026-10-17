/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

/*
 * Early kernel log: plain text to the serial console. Log levels and a
 * formatted interface come in Phase 3; boot tests match on the exact lines
 * written here, so keep their wording stable.
 */

#ifndef SMY_KERNEL_CORE_LOG_H
#define SMY_KERNEL_CORE_LOG_H

#include <stdbool.h>
#include <stdint.h>

/* Bring up the serial console; returns false if no serial port answered. */
bool log_init(void);

void log_write(const char *s);
void log_write_dec(uint64_t value);
void log_write_hex(uint64_t value); /* with "0x" prefix */

#endif /* SMY_KERNEL_CORE_LOG_H */
