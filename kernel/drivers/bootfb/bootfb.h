/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

/*
 * Boot framebuffer: the linear framebuffer the bootloader set up (UEFI GOP or
 * VBE). This is NOT a graphics driver: no mode setting, no acceleration, no
 * vsync. It only fills rectangles in the pixel format it was given.
 * Host-testable: it never touches hardware except through `pixels`.
 */

#ifndef SMY_KERNEL_DRIVERS_BOOTFB_BOOTFB_H
#define SMY_KERNEL_DRIVERS_BOOTFB_BOOTFB_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct bootfb_format {
    uint8_t red_shift;
    uint8_t red_size;
    uint8_t green_shift;
    uint8_t green_size;
    uint8_t blue_shift;
    uint8_t blue_size;
};

struct bootfb {
    uint32_t *pixels;
    uint32_t width;
    uint32_t height;
    uint32_t stride; /* pixels per row (pitch in bytes / 4) */
    struct bootfb_format format;
};

/*
 * Validate the bootloader's description and fill `fb`. Only 32 bits per
 * pixel with a pitch that is a multiple of 4 is supported; anything else
 * returns false and the kernel continues without graphics.
 */
bool bootfb_init(struct bootfb *fb, void *address, uint64_t width, uint64_t height,
                 uint64_t pitch_bytes, uint16_t bpp, const struct bootfb_format *format);

/* Encode an 8-bit-per-channel colour in the framebuffer's pixel format. */
uint32_t bootfb_rgb(const struct bootfb *fb, uint8_t r, uint8_t g, uint8_t b);

/* Fill a rectangle; parts outside the framebuffer are clipped. */
void bootfb_fill_rect(const struct bootfb *fb, uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                      uint32_t color);

/*
 * Draw a run-length-encoded RGB image (runs of {count 1..255, r, g, b},
 * row by row; drivers/bootfb/boot_logo.h) with its top-left corner at
 * (x, y); parts outside the framebuffer are clipped. The data is validated
 * first: a zero-length run, a truncated run or a pixel count other than
 * w * h returns false and draws nothing.
 */
bool bootfb_blit_rle(const struct bootfb *fb, uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                     const uint8_t *rle, size_t size);

#endif /* SMY_KERNEL_DRIVERS_BOOTFB_BOOTFB_H */
