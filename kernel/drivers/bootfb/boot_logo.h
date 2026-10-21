/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

/*
 * The SMY-OS boot logo as run-length-encoded RGB pixels, generated at build
 * time from design/brand/logos/smy-boot-logo.svg by
 * tools/design/boot_logo.py (one entry per size, smallest first).
 */

#ifndef SMY_KERNEL_DRIVERS_BOOTFB_BOOT_LOGO_H
#define SMY_KERNEL_DRIVERS_BOOTFB_BOOT_LOGO_H

#include <stddef.h>
#include <stdint.h>

struct boot_logo {
    uint32_t width;
    uint32_t height;
    const uint8_t *rle; /* runs of {count, red, green, blue} */
    size_t rle_size;
};

extern const struct boot_logo boot_logos[];
extern const unsigned boot_logo_count;

#endif /* SMY_KERNEL_DRIVERS_BOOTFB_BOOT_LOGO_H */
