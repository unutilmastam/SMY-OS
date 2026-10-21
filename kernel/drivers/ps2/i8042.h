/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

/*
 * PS/2 controller (i8042) and keyboard on its first port
 * (docs/kernel/input.md). x86 legacy hardware reached through port I/O
 * (ADR 0008 exception to the portability rule, like the 16550 UART).
 */

#ifndef SMY_KERNEL_DRIVERS_PS2_I8042_H
#define SMY_KERNEL_DRIVERS_PS2_I8042_H

#include "drivers/input/keyboard.h"

#include <stdbool.h>
#include <stdint.h>

/*
 * Probe and configure the controller (self-test, translation to scancode
 * set 1, port 1 interrupt on) and attach ISA IRQ 1. False, with a log line,
 * if there is no working controller; every wait is bounded.
 */
bool ps2_keyboard_init(void);

/* Next key event, if any (called with interrupts enabled or disabled). */
bool ps2_keyboard_read(struct key_event *out);

/* Events dropped because the ring was full. */
uint32_t ps2_keyboard_dropped(void);

#endif /* SMY_KERNEL_DRIVERS_PS2_I8042_H */
