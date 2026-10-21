/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

/* Host tests for kernel/drivers/bootfb/bootfb.c on an in-memory framebuffer. */

#include "drivers/bootfb/bootfb.h"
#include "harness/smytest.h"

#include <stdint.h>
#include <string.h>

#define FB_W 8u
#define FB_H 6u
#define FB_STRIDE 10u                      /* wider than FB_W: padding pixels must stay untouched */
#define FB_PITCH ((uint64_t)FB_STRIDE * 4) /* bytes per row */
#define PAD 0xDEADBEEFu

/* The common UEFI GOP format: 0x00RRGGBB. */
static const struct bootfb_format xrgb8888 = {.red_shift = 16,
                                              .red_size = 8,
                                              .green_shift = 8,
                                              .green_size = 8,
                                              .blue_shift = 0,
                                              .blue_size = 8};

static uint32_t pixels[FB_STRIDE * FB_H];

static struct bootfb make_fb(void)
{
    struct bootfb fb;

    for (size_t i = 0; i < (size_t)FB_STRIDE * FB_H; i++) {
        pixels[i] = PAD;
    }
    SMY_EXPECT(bootfb_init(&fb, pixels, FB_W, FB_H, FB_PITCH, 32, &xrgb8888));
    return fb;
}

static unsigned count_color(uint32_t color)
{
    unsigned n = 0;

    for (uint32_t y = 0; y < FB_H; y++) {
        for (uint32_t x = 0; x < FB_W; x++) {
            n += pixels[y * FB_STRIDE + x] == color;
        }
    }
    return n;
}

SMY_TEST(bootfb_init_accepts_valid_and_rejects_invalid_modes)
{
    struct bootfb fb;
    struct bootfb_format bad = xrgb8888;

    SMY_EXPECT(bootfb_init(&fb, pixels, FB_W, FB_H, FB_PITCH, 32, &xrgb8888));
    SMY_EXPECT_EQ_U64(fb.stride, FB_STRIDE);
    SMY_EXPECT(!bootfb_init(&fb, pixels, FB_W, FB_H, FB_PITCH, 24, &xrgb8888));
    SMY_EXPECT(!bootfb_init(&fb, pixels, FB_W, FB_H, (uint64_t)FB_W * 4 - 4, 32, &xrgb8888));
    SMY_EXPECT(!bootfb_init(&fb, pixels, FB_W, FB_H, (uint64_t)FB_W * 4 + 2, 32, &xrgb8888));
    SMY_EXPECT(!bootfb_init(&fb, pixels, 0, FB_H, FB_PITCH, 32, &xrgb8888));
    SMY_EXPECT(!bootfb_init(&fb, NULL, FB_W, FB_H, FB_PITCH, 32, &xrgb8888));
    bad.red_shift = 30; /* 30 + 8 > 32 */
    SMY_EXPECT(!bootfb_init(&fb, pixels, FB_W, FB_H, FB_PITCH, 32, &bad));
}

SMY_TEST(bootfb_rgb_encodes_channel_masks)
{
    struct bootfb fb = make_fb();
    struct bootfb rgb565_like = fb;

    SMY_EXPECT_EQ_U64(bootfb_rgb(&fb, 0x12, 0x34, 0x56), 0x123456);
    /* Fewer bits per channel keep the most significant bits. */
    rgb565_like.format = (struct bootfb_format){.red_shift = 11,
                                                .red_size = 5,
                                                .green_shift = 5,
                                                .green_size = 6,
                                                .blue_shift = 0,
                                                .blue_size = 5};
    SMY_EXPECT_EQ_U64(bootfb_rgb(&rgb565_like, 0xFF, 0xFF, 0xFF), 0xFFFF);
    SMY_EXPECT_EQ_U64(bootfb_rgb(&rgb565_like, 0x80, 0x00, 0x00), 0x8000);
}

SMY_TEST(bootfb_fill_rect_fills_exact_area)
{
    struct bootfb fb = make_fb();

    bootfb_fill_rect(&fb, 2, 1, 3, 2, 0x00AA00);
    SMY_EXPECT_EQ_U64(count_color(0x00AA00), 6);
    SMY_EXPECT_EQ_U64(pixels[1 * FB_STRIDE + 2], 0x00AA00);
    SMY_EXPECT_EQ_U64(pixels[2 * FB_STRIDE + 4], 0x00AA00);
    SMY_EXPECT_EQ_U64(pixels[1 * FB_STRIDE + 5], PAD);
    SMY_EXPECT_EQ_U64(pixels[3 * FB_STRIDE + 2], PAD);
}

SMY_TEST(bootfb_fill_rect_clips_and_keeps_padding)
{
    struct bootfb fb = make_fb();

    /* Overhangs right and bottom; huge sizes must not overflow. */
    bootfb_fill_rect(&fb, 6, 4, UINT32_MAX, UINT32_MAX, 0x111111);
    SMY_EXPECT_EQ_U64(count_color(0x111111), 2 * 2);
    for (uint32_t y = 0; y < FB_H; y++) {
        for (uint32_t x = FB_W; x < FB_STRIDE; x++) {
            SMY_EXPECT_EQ_U64(pixels[y * FB_STRIDE + x], PAD);
        }
    }
    /* Entirely outside: no effect. */
    bootfb_fill_rect(&fb, FB_W, 0, 4, 4, 0x222222);
    bootfb_fill_rect(&fb, 0, FB_H, 4, 4, 0x222222);
    SMY_EXPECT_EQ_U64(count_color(0x222222), 0);
    /* Whole screen. */
    bootfb_fill_rect(&fb, 0, 0, FB_W, FB_H, 0x333333);
    SMY_EXPECT_EQ_U64(count_color(0x333333), FB_W * FB_H);
}

SMY_TEST(bootfb_blit_rle_draws_runs_across_rows)
{
    struct bootfb fb = make_fb();
    /* 3 x 2 image: 4 red pixels (wrapping into row 2), then 2 blue. */
    const uint8_t rle[] = {4, 0xFF, 0x00, 0x00, 2, 0x00, 0x00, 0xFF};

    SMY_EXPECT(bootfb_blit_rle(&fb, 1, 2, 3, 2, rle, sizeof rle));
    SMY_EXPECT_EQ_U64(pixels[2 * FB_STRIDE + 1], 0xFF0000);
    SMY_EXPECT_EQ_U64(pixels[2 * FB_STRIDE + 3], 0xFF0000);
    SMY_EXPECT_EQ_U64(pixels[3 * FB_STRIDE + 1], 0xFF0000);
    SMY_EXPECT_EQ_U64(pixels[3 * FB_STRIDE + 2], 0x0000FF);
    SMY_EXPECT_EQ_U64(pixels[3 * FB_STRIDE + 3], 0x0000FF);
    SMY_EXPECT_EQ_U64(pixels[2 * FB_STRIDE + 4], PAD);
    SMY_EXPECT_EQ_U64(count_color(0xFF0000), 4);
    SMY_EXPECT_EQ_U64(count_color(0x0000FF), 2);
}

SMY_TEST(bootfb_blit_rle_clips_at_the_edges)
{
    struct bootfb fb = make_fb();
    const uint8_t rle[] = {255, 0x11, 0x22, 0x33, 145, 0x11, 0x22, 0x33}; /* 20 x 20 */

    SMY_EXPECT(bootfb_blit_rle(&fb, FB_W - 2, FB_H - 3, 20, 20, rle, sizeof rle));
    SMY_EXPECT_EQ_U64(count_color(0x112233), 2 * 3);
    for (uint32_t y = 0; y < FB_H; y++) {
        for (uint32_t x = FB_W; x < FB_STRIDE; x++) {
            SMY_EXPECT_EQ_U64(pixels[y * FB_STRIDE + x], PAD);
        }
    }
}

SMY_TEST(bootfb_blit_rle_rejects_malformed_data_without_drawing)
{
    struct bootfb fb = make_fb();
    const uint8_t too_few[] = {5, 1, 2, 3};              /* 5 of 6 pixels */
    const uint8_t too_many[] = {7, 1, 2, 3};             /* 7 of 6 */
    const uint8_t zero_run[] = {0, 1, 2, 3, 6, 1, 2, 3}; /* zero-length run */
    const uint8_t truncated[] = {6, 1, 2};               /* not a whole run */

    SMY_EXPECT(!bootfb_blit_rle(&fb, 0, 0, 3, 2, too_few, sizeof too_few));
    SMY_EXPECT(!bootfb_blit_rle(&fb, 0, 0, 3, 2, too_many, sizeof too_many));
    SMY_EXPECT(!bootfb_blit_rle(&fb, 0, 0, 3, 2, zero_run, sizeof zero_run));
    SMY_EXPECT(!bootfb_blit_rle(&fb, 0, 0, 3, 2, truncated, sizeof truncated));
    SMY_EXPECT(!bootfb_blit_rle(&fb, 0, 0, 0, 2, too_few, sizeof too_few));
    SMY_EXPECT(!bootfb_blit_rle(&fb, 0, 0, 3, 2, NULL, 4));
    SMY_EXPECT_EQ_U64(count_color(0x010203), 0);
}
