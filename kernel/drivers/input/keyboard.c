/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

#include "drivers/input/keyboard.h"

#include <stddef.h>

enum {
    STATE_IDLE = 0,
    STATE_E0,
    STATE_E1_1, /* after E1: expect 1D or 9D */
    STATE_E1_2, /* after E1 1D: expect 45 (press) */
    STATE_E1_3, /* expect E1 */
    STATE_E1_4, /* expect 9D */
    STATE_E1_5, /* expect C5 (release) */
};

/* Set 1 make code -> HID usage, codes 0x00..0x58 (0 = unused). */
static const uint8_t base_map[0x59] = {
    [0x01] = 0x29, /* Escape */
    [0x02] = 0x1E, [0x03] = 0x1F, [0x04] = 0x20, [0x05] = 0x21, [0x06] = 0x22, [0x07] = 0x23,
    [0x08] = 0x24, [0x09] = 0x25, [0x0A] = 0x26, [0x0B] = 0x27, /* 1..9 0 */
    [0x0C] = 0x2D, [0x0D] = 0x2E, [0x0E] = 0x2A, [0x0F] = 0x2B, /* - = Backspace Tab */
    [0x10] = 0x14, [0x11] = 0x1A, [0x12] = 0x08, [0x13] = 0x15, [0x14] = 0x17, /* Q W E R T */
    [0x15] = 0x1C, [0x16] = 0x18, [0x17] = 0x0C, [0x18] = 0x12, [0x19] = 0x13, /* Y U I O P */
    [0x1A] = 0x2F, [0x1B] = 0x30, [0x1C] = 0x28, [0x1D] = 0xE0,                /* [ ] Enter LCtrl */
    [0x1E] = 0x04, [0x1F] = 0x16, [0x20] = 0x07, [0x21] = 0x09, [0x22] = 0x0A, /* A S D F G */
    [0x23] = 0x0B, [0x24] = 0x0D, [0x25] = 0x0E, [0x26] = 0x0F,                /* H J K L */
    [0x27] = 0x33, [0x28] = 0x34, [0x29] = 0x35, [0x2A] = 0xE1, [0x2B] = 0x31, /* ; ' ` LShift \ */
    [0x2C] = 0x1D, [0x2D] = 0x1B, [0x2E] = 0x06, [0x2F] = 0x19, [0x30] = 0x05, /* Z X C V B */
    [0x31] = 0x11, [0x32] = 0x10, [0x33] = 0x36, [0x34] = 0x37, [0x35] = 0x38, /* N M , . / */
    [0x36] = 0xE5, [0x37] = 0x55, [0x38] = 0xE2, [0x39] = 0x2C, [0x3A] = 0x39, /* RShift KP* LAlt
                                                                                  Space Caps */
    [0x3B] = 0x3A, [0x3C] = 0x3B, [0x3D] = 0x3C, [0x3E] = 0x3D, [0x3F] = 0x3E, /* F1..F5 */
    [0x40] = 0x3F, [0x41] = 0x40, [0x42] = 0x41, [0x43] = 0x42, [0x44] = 0x43, /* F6..F10 */
    [0x45] = 0x53, [0x46] = 0x47,                               /* NumLock ScrollLock */
    [0x47] = 0x5F, [0x48] = 0x60, [0x49] = 0x61, [0x4A] = 0x56, /* KP7 KP8 KP9 KP- */
    [0x4B] = 0x5C, [0x4C] = 0x5D, [0x4D] = 0x5E, [0x4E] = 0x57, /* KP4 KP5 KP6 KP+ */
    [0x4F] = 0x59, [0x50] = 0x5A, [0x51] = 0x5B, [0x52] = 0x62, [0x53] = 0x63, /* KP1 KP2 KP3 KP0
                                                                                  KP. */
    [0x56] = 0x64, [0x57] = 0x44, [0x58] = 0x45, /* non-US \ F11 F12 */
};

/* E0-prefixed make codes. */
static uint16_t extended_key(uint8_t code)
{
    switch (code) {
    case 0x1C:
        return 0x58; /* keypad Enter */
    case 0x1D:
        return 0xE4; /* right Ctrl */
    case 0x35:
        return 0x54; /* keypad / */
    case 0x37:
        return 0x46; /* Print Screen */
    case 0x38:
        return 0xE6; /* right Alt (AltGr) */
    case 0x47:
        return 0x4A; /* Home */
    case 0x48:
        return 0x52; /* Up */
    case 0x49:
        return 0x4B; /* Page Up */
    case 0x4B:
        return 0x50; /* Left */
    case 0x4D:
        return 0x4F; /* Right */
    case 0x4F:
        return 0x4D; /* End */
    case 0x50:
        return 0x51; /* Down */
    case 0x51:
        return 0x4E; /* Page Down */
    case 0x52:
        return 0x49; /* Insert */
    case 0x53:
        return 0x4C; /* Delete */
    case 0x5B:
        return 0xE3; /* left GUI */
    case 0x5C:
        return 0xE7; /* right GUI */
    case 0x5D:
        return 0x65; /* Application (menu) */
    default:
        return KEY_NONE; /* includes the fake shifts 2A/36 around Print Screen */
    }
}

void kbd_set1_init(struct kbd_set1 *d)
{
    d->state = STATE_IDLE;
}

static bool emit(uint16_t key, bool pressed, struct key_event *out)
{
    if (key == KEY_NONE) {
        return false;
    }
    out->key = key;
    out->pressed = pressed;
    return true;
}

bool kbd_set1_feed(struct kbd_set1 *d, uint8_t byte, struct key_event *out)
{
    const uint8_t code = byte & 0x7F;
    const bool pressed = (byte & 0x80) == 0;

    switch (d->state) {
    case STATE_E0:
        d->state = STATE_IDLE;
        return emit(extended_key(code), pressed, out);
    case STATE_E1_1:
        d->state = byte == 0x1D ? STATE_E1_2 : STATE_IDLE;
        return false;
    case STATE_E1_2:
        d->state = STATE_IDLE;
        if (byte == 0x45) {
            d->state = STATE_E1_3;
            return emit(KEY_PAUSE, true, out);
        }
        return false;
    case STATE_E1_3:
        d->state = byte == 0xE1 ? STATE_E1_4 : STATE_IDLE;
        return false;
    case STATE_E1_4:
        d->state = byte == 0x9D ? STATE_E1_5 : STATE_IDLE;
        return false;
    case STATE_E1_5:
        d->state = STATE_IDLE;
        if (byte != 0xC5) {
            return false;
        }
        out->key = KEY_PAUSE; /* Pause has no real break code: report it here */
        out->pressed = false;
        return true;
    default:
        break;
    }
    switch (byte) {
    case 0xE0:
        d->state = STATE_E0;
        return false;
    case 0xE1:
        d->state = STATE_E1_1;
        return false;
    case 0x00: /* key detection error / overrun */
    case 0xFA: /* acknowledge */
    case 0xFE: /* resend */
    case 0xFF: /* key detection error / overrun */
        return false;
    default:
        return code < sizeof base_map && emit(base_map[code], pressed, out);
    }
}

void key_ring_init(struct key_ring *r)
{
    r->head = 0;
    r->tail = 0;
    r->dropped = 0;
}

bool key_ring_push(struct key_ring *r, struct key_event ev)
{
    const uint32_t head = r->head;

    if (head - r->tail == KEY_RING_SIZE) {
        r->dropped++;
        return false;
    }
    r->events[head % KEY_RING_SIZE] = ev;
    __atomic_store_n(&r->head, head + 1, __ATOMIC_RELEASE);
    return true;
}

bool key_ring_pop(struct key_ring *r, struct key_event *out)
{
    const uint32_t tail = r->tail;

    if (__atomic_load_n(&r->head, __ATOMIC_ACQUIRE) == tail) {
        return false;
    }
    *out = r->events[tail % KEY_RING_SIZE];
    __atomic_store_n(&r->tail, tail + 1, __ATOMIC_RELEASE);
    return true;
}
