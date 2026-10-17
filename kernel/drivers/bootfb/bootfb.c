/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

#include "drivers/bootfb/bootfb.h"

#include <stddef.h>

static bool channel_fits(uint8_t shift, uint8_t size)
{
    return size >= 1 && size <= 8 && shift + size <= 32;
}

bool bootfb_init(struct bootfb *fb, void *address, uint64_t width, uint64_t height,
                 uint64_t pitch_bytes, uint16_t bpp, const struct bootfb_format *format)
{
    if (fb == NULL || address == NULL || format == NULL) {
        return false;
    }
    if (bpp != 32 || width == 0 || height == 0 || width > UINT32_MAX || height > UINT32_MAX) {
        return false;
    }
    if (pitch_bytes % 4 != 0 || pitch_bytes / 4 < width || pitch_bytes / 4 > UINT32_MAX) {
        return false;
    }
    if (!channel_fits(format->red_shift, format->red_size) ||
        !channel_fits(format->green_shift, format->green_size) ||
        !channel_fits(format->blue_shift, format->blue_size)) {
        return false;
    }

    fb->pixels = address;
    fb->width = (uint32_t)width;
    fb->height = (uint32_t)height;
    fb->stride = (uint32_t)(pitch_bytes / 4);
    fb->format = *format;
    return true;
}

/* Scale an 8-bit channel down to `size` bits and move it into place. */
static uint32_t encode_channel(uint8_t value, uint8_t shift, uint8_t size)
{
    return ((uint32_t)value >> (8u - size)) << shift;
}

uint32_t bootfb_rgb(const struct bootfb *fb, uint8_t r, uint8_t g, uint8_t b)
{
    const struct bootfb_format *f = &fb->format;

    return encode_channel(r, f->red_shift, f->red_size) |
           encode_channel(g, f->green_shift, f->green_size) |
           encode_channel(b, f->blue_shift, f->blue_size);
}

void bootfb_fill_rect(const struct bootfb *fb, uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                      uint32_t color)
{
    if (x >= fb->width || y >= fb->height) {
        return;
    }
    /* Clip without overflow: compare against the remaining space. */
    if (w > fb->width - x) {
        w = fb->width - x;
    }
    if (h > fb->height - y) {
        h = fb->height - y;
    }
    for (uint32_t row = 0; row < h; row++) {
        uint32_t *line = fb->pixels + (size_t)(y + row) * fb->stride + x;
        for (uint32_t col = 0; col < w; col++) {
            line[col] = color;
        }
    }
}
