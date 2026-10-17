/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

/* 16550-compatible UART on a legacy I/O port (COM1), polled, 115200 8N1. */

#ifndef SMY_KERNEL_DRIVERS_SERIAL_UART16550_H
#define SMY_KERNEL_DRIVERS_SERIAL_UART16550_H

#include <stdbool.h>
#include <stdint.h>

#define UART16550_COM1 0x3F8

/*
 * Initialise the UART at `base`. Returns false if no working UART answers
 * (loopback test fails); output is then silently dropped, so a machine
 * without a serial port still boots.
 */
bool uart16550_init(uint16_t base);

/* Write one byte; "\n" is sent as "\r\n" for terminals. */
void uart16550_putc(char c);

#endif /* SMY_KERNEL_DRIVERS_SERIAL_UART16550_H */
