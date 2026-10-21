/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

/* Host tests for kernel/drivers/input/keyboard.c. */

#include "drivers/input/keyboard.h"
#include "harness/smytest.h"

#include <stddef.h>

/* Feed bytes; collect up to `max` events. */
static size_t feed(struct kbd_set1 *d, const uint8_t *bytes, size_t n, struct key_event *ev,
                   size_t max)
{
    size_t count = 0;

    for (size_t i = 0; i < n; i++) {
        struct key_event e;

        if (kbd_set1_feed(d, bytes[i], &e) && count < max) {
            ev[count++] = e;
        }
    }
    return count;
}

SMY_TEST(keyboard_set1_plain_keys_press_and_release)
{
    struct kbd_set1 d;
    struct key_event ev[8];
    /* s down, s up, m down, m up, y down, y up, Enter down, Enter up */
    const uint8_t bytes[] = {0x1F, 0x9F, 0x32, 0xB2, 0x15, 0x95, 0x1C, 0x9C};

    kbd_set1_init(&d);
    SMY_EXPECT_EQ_U64(feed(&d, bytes, sizeof bytes, ev, 8), 8);
    SMY_EXPECT_EQ_U64(ev[0].key, KEY_S);
    SMY_EXPECT(ev[0].pressed);
    SMY_EXPECT_EQ_U64(ev[1].key, KEY_S);
    SMY_EXPECT(!ev[1].pressed);
    SMY_EXPECT_EQ_U64(ev[2].key, KEY_M);
    SMY_EXPECT_EQ_U64(ev[4].key, KEY_Y);
    SMY_EXPECT_EQ_U64(ev[6].key, KEY_ENTER);
    SMY_EXPECT(!ev[7].pressed);
}

SMY_TEST(keyboard_set1_extended_keys_and_modifiers)
{
    struct kbd_set1 d;
    struct key_event ev[8];
    /* LShift down, E0 Up down, E0 Up up, LShift up, E0 RAlt down/up, E0 KP Enter down */
    const uint8_t bytes[] = {0x2A, 0xE0, 0x48, 0xE0, 0xC8, 0xAA,
                             0xE0, 0x38, 0xE0, 0xB8, 0xE0, 0x1C};

    kbd_set1_init(&d);
    SMY_EXPECT_EQ_U64(feed(&d, bytes, sizeof bytes, ev, 8), 7);
    SMY_EXPECT_EQ_U64(ev[0].key, KEY_LEFT_SHIFT);
    SMY_EXPECT_EQ_U64(ev[1].key, KEY_UP);
    SMY_EXPECT(ev[1].pressed);
    SMY_EXPECT_EQ_U64(ev[2].key, KEY_UP);
    SMY_EXPECT(!ev[2].pressed);
    SMY_EXPECT_EQ_U64(ev[3].key, KEY_LEFT_SHIFT); /* 0xAA is LShift up, not "self-test OK" */
    SMY_EXPECT(!ev[3].pressed);
    SMY_EXPECT_EQ_U64(ev[4].key, KEY_RIGHT_ALT);
    SMY_EXPECT_EQ_U64(ev[6].key, KEY_KP_ENTER);
}

SMY_TEST(keyboard_set1_print_screen_and_pause_sequences)
{
    struct kbd_set1 d;
    struct key_event ev[8];
    /* Print Screen make E0 2A E0 37, break E0 B7 E0 AA; Pause E1 1D 45 E1 9D C5 */
    const uint8_t bytes[] = {0xE0, 0x2A, 0xE0, 0x37, 0xE0, 0xB7, 0xE0,
                             0xAA, 0xE1, 0x1D, 0x45, 0xE1, 0x9D, 0xC5};

    kbd_set1_init(&d);
    SMY_EXPECT_EQ_U64(feed(&d, bytes, sizeof bytes, ev, 8), 4);
    SMY_EXPECT_EQ_U64(ev[0].key, 0x46);
    SMY_EXPECT(ev[0].pressed);
    SMY_EXPECT_EQ_U64(ev[1].key, 0x46);
    SMY_EXPECT(!ev[1].pressed);
    SMY_EXPECT_EQ_U64(ev[2].key, KEY_PAUSE);
    SMY_EXPECT(ev[2].pressed);
    SMY_EXPECT_EQ_U64(ev[3].key, KEY_PAUSE);
    SMY_EXPECT(!ev[3].pressed);
}

SMY_TEST(keyboard_set1_ignores_replies_and_unknown_codes)
{
    struct kbd_set1 d;
    struct key_event ev[4];
    /* ACK, resend, overrun, unknown 0x59, broken Pause (E1 1D 99), then A */
    const uint8_t bytes[] = {0xFA, 0xFE, 0xFF, 0x00, 0x59, 0xE1, 0x1D, 0x99, 0x1E};

    kbd_set1_init(&d);
    SMY_EXPECT_EQ_U64(feed(&d, bytes, sizeof bytes, ev, 4), 1);
    SMY_EXPECT_EQ_U64(ev[0].key, KEY_A);
}

SMY_TEST(keyboard_every_set1_code_maps_to_a_distinct_key)
{
    struct kbd_set1 d;
    uint8_t seen[256] = {0};
    unsigned mapped = 0;

    for (unsigned code = 0x01; code <= 0x58; code++) {
        struct key_event e;

        kbd_set1_init(&d);
        if (kbd_set1_feed(&d, (uint8_t)code, &e)) {
            SMY_EXPECT(e.pressed);
            if (e.key == KEY_NONE || e.key >= sizeof seen) {
                SMY_EXPECT(e.key != KEY_NONE && e.key < sizeof seen);
                continue;
            }
            SMY_EXPECT_EQ_U64(seen[e.key], 0); /* no two codes share a key */
            seen[e.key] = 1;
            mapped++;
        }
    }
    SMY_EXPECT_EQ_U64(mapped, 86); /* 0x01-0x53 plus 0x56-0x58 */
}

SMY_TEST(keyboard_ring_keeps_order_and_drops_when_full)
{
    struct key_ring r;
    struct key_event e;

    key_ring_init(&r);
    SMY_EXPECT(!key_ring_pop(&r, &e));
    for (unsigned i = 0; i < KEY_RING_SIZE; i++) {
        SMY_EXPECT(key_ring_push(&r, (struct key_event){.key = (uint16_t)i, .pressed = true}));
    }
    SMY_EXPECT(!key_ring_push(&r, (struct key_event){.key = 999, .pressed = true}));
    SMY_EXPECT_EQ_U64(r.dropped, 1);
    for (unsigned i = 0; i < KEY_RING_SIZE; i++) {
        SMY_EXPECT(key_ring_pop(&r, &e));
        SMY_EXPECT_EQ_U64(e.key, i);
    }
    SMY_EXPECT(!key_ring_pop(&r, &e));
    /* Wrap-around of the 32-bit indices. */
    r.head = r.tail = 0xFFFFFFFEu;
    SMY_EXPECT(key_ring_push(&r, (struct key_event){.key = 7, .pressed = false}));
    SMY_EXPECT(key_ring_push(&r, (struct key_event){.key = 8, .pressed = false}));
    SMY_EXPECT(key_ring_push(&r, (struct key_event){.key = 9, .pressed = false}));
    SMY_EXPECT(key_ring_pop(&r, &e) && e.key == 7);
    SMY_EXPECT(key_ring_pop(&r, &e) && e.key == 8);
    SMY_EXPECT(key_ring_pop(&r, &e) && e.key == 9);
}
