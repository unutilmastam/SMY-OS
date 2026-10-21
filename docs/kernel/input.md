# Keyboard input

Status: **Implemented, Unit-tested, Emulator-tested** (Phase 7; keystrokes
injected through the QEMU monitor on BIOS and UEFI).

Code: `kernel/drivers/input/keyboard.{h,c}` (key codes, scancode decoder,
event ring; host-tested) and `kernel/drivers/ps2/i8042.{h,c}` (PS/2
controller, x86 port I/O).

## Key events

```c
struct key_event { uint16_t key; bool pressed; };   /* key = USB HID usage, page 0x07 */
bool ps2_keyboard_read(struct key_event *out);      /* next event, if any */
```

A key event names the **physical key**, independent of the keyboard layout
(for example `KEY_S` = 0x16). Turning keys into characters for Uzbek Latin,
Uzbek Cyrillic, English or Russian layouts belongs to user space (input
method, Phase 20+).

## PS/2 path

1. Init (every wait bounded, so a missing controller is logged, not a hang):
   - disable both ports and flush;
   - controller self-test (0x55);
   - port 1 test;
   - configuration: port 1 interrupt on, translation on (scancode set 1),
     port 2 off;
   - enable port 1 and attach ISA IRQ 1.
2. IRQ 1: read every waiting byte, feed the set 1 decoder and push the
   finished events into the ring. The ring holds 64 events, has a single
   producer and a single consumer, and needs no locks. When it is full,
   new events are dropped and counted.
3. The decoder handles plain codes, `E0` extended keys, `E1 1D 45 E1 9D C5`
   (Pause), the fake shifts around Print Screen and controller replies
   (`FA`, `FE`, `00`, `FF`). Note that `AA` is the break code of the left
   Shift, not a self-test reply.

## Tests

- **Host** (`test_keyboard.c`, 6 tests):
  - plain, extended and modifier keys;
  - Print Screen and Pause sequences;
  - ignored replies;
  - all 86 base codes map to distinct keys;
  - ring order, overflow and index wrap-around.
- **QEMU** (`make ktest-keyboard-bios ktest-keyboard-uefi`): the test logs
  "waiting for keyboard input". `tests/integration/sendkeys.py` then types
  `s m y up ret` through the QEMU monitor, and the kernel must receive
  exactly these 10 events (press and release, in order).
- **Negative checks:**
  - a wrong key (`x` instead of `y`) fails with the exact event;
  - no keys at all fails after the 20 s timeout.

## Limits

- PS/2 only (USB keyboards work through the firmware's PS/2 emulation
  until the USB stack, Phase 26–27).
- No keyboard LEDs, typematic settings or scancode set 2 decoding.
- Key events are only collected; nothing in the kernel consumes them
  outside tests yet (console: Phase 13).
