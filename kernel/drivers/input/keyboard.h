/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

/*
 * Keyboard input core (docs/kernel/input.md): layout-independent key events
 * and the PS/2 scancode set 1 decoder. Pure logic, host-testable.
 *
 * Key codes are USB HID usage IDs (HID Usage Tables 1.5, page 0x07
 * "Keyboard/Keypad"): they name the physical key position, not the
 * character. Turning keys into characters (layouts: Uzbek Latin, Uzbek
 * Cyrillic, English, Russian ...) is a later, user-space step.
 */

#ifndef SMY_KERNEL_DRIVERS_INPUT_KEYBOARD_H
#define SMY_KERNEL_DRIVERS_INPUT_KEYBOARD_H

#include <stdbool.h>
#include <stdint.h>

/* A few HID usages used by tests and the kernel; the full set is in the
 * decoder tables. */
enum {
    KEY_NONE = 0x00,
    KEY_A = 0x04,
    KEY_M = 0x10,
    KEY_S = 0x16,
    KEY_Y = 0x1C,
    KEY_1 = 0x1E,
    KEY_ENTER = 0x28,
    KEY_ESCAPE = 0x29,
    KEY_SPACE = 0x2C,
    KEY_PAUSE = 0x48,
    KEY_RIGHT = 0x4F,
    KEY_LEFT = 0x50,
    KEY_DOWN = 0x51,
    KEY_UP = 0x52,
    KEY_KP_ENTER = 0x58,
    KEY_LEFT_CTRL = 0xE0,
    KEY_LEFT_SHIFT = 0xE1,
    KEY_LEFT_ALT = 0xE2,
    KEY_RIGHT_CTRL = 0xE4,
    KEY_RIGHT_ALT = 0xE6,
};

struct key_event {
    uint16_t key; /* HID usage */
    bool pressed; /* false = released */
};

/* Scancode set 1 decoder state (prefix bytes E0 / E1 in progress). */
struct kbd_set1 {
    uint8_t state;
};

void kbd_set1_init(struct kbd_set1 *d);

/*
 * Feed one byte from the controller. Returns true and fills `out` when the
 * byte completes a key event. Prefixes, controller replies (ACK, resend,
 * errors), "fake shift" bytes around Print Screen and unknown codes return
 * false.
 */
bool kbd_set1_feed(struct kbd_set1 *d, uint8_t byte, struct key_event *out);

/*
 * Single-producer/single-consumer ring of key events: the interrupt handler
 * pushes, the reader pops. When full, new events are dropped and counted
 * (never overwriting unread ones).
 */
#define KEY_RING_SIZE 64u /* power of two */

struct key_ring {
    struct key_event events[KEY_RING_SIZE];
    volatile uint32_t head; /* next write (producer) */
    volatile uint32_t tail; /* next read (consumer) */
    volatile uint32_t dropped;
};

void key_ring_init(struct key_ring *r);
bool key_ring_push(struct key_ring *r, struct key_event ev);
bool key_ring_pop(struct key_ring *r, struct key_event *out);

#endif /* SMY_KERNEL_DRIVERS_INPUT_KEYBOARD_H */
