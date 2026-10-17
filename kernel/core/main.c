/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

/*
 * SMY Kernel entry: bring up the serial log, check the boot protocol, install
 * the CPU's descriptor tables and exception handlers, paint the boot
 * framebuffer, then either run the self-tests (smy.ktest=...) or report
 * "boot OK" and halt.
 */

#include "arch/cpu.h"
#include "core/cmdline.h"
#include "core/compiler.h"
#include "core/ktest.h"
#include "core/log.h"
#include "core/panic.h"
#include "drivers/bootfb/bootfb.h"

#include <limine.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Limine requests. They live in their own section between the start and end
 * markers (Limine protocol, "Requests Delimiters"); the bootloader fills in
 * the `response` pointers before jumping to _start.
 */
SMY_USED SMY_SECTION(".limine_requests_start") static volatile uint64_t limine_requests_start[] =
    LIMINE_REQUESTS_START_MARKER;

SMY_USED SMY_SECTION(".limine_requests") static volatile uint64_t limine_base_revision[] =
    LIMINE_BASE_REVISION(6);

SMY_USED SMY_SECTION(".limine_requests") static volatile struct limine_bootloader_info_request
    bootloader_info_request = {.id = LIMINE_BOOTLOADER_INFO_REQUEST_ID, .revision = 0};

SMY_USED SMY_SECTION(".limine_requests") static volatile struct limine_firmware_type_request
    firmware_type_request = {.id = LIMINE_FIRMWARE_TYPE_REQUEST_ID, .revision = 0};

SMY_USED SMY_SECTION(".limine_requests") static volatile struct limine_framebuffer_request
    framebuffer_request = {.id = LIMINE_FRAMEBUFFER_REQUEST_ID, .revision = 0};

SMY_USED SMY_SECTION(".limine_requests") static volatile struct limine_executable_cmdline_request
    cmdline_request = {.id = LIMINE_EXECUTABLE_CMDLINE_REQUEST_ID, .revision = 0};

SMY_USED SMY_SECTION(".limine_requests_end") static volatile uint64_t limine_requests_end[] =
    LIMINE_REQUESTS_END_MARKER;

SMY_NORETURN void kmain(void); /* called from arch/x86_64/entry.S */

static const char *firmware_name(uint64_t type)
{
    switch (type) {
    case LIMINE_FIRMWARE_TYPE_X86BIOS:
        return "BIOS";
    case LIMINE_FIRMWARE_TYPE_EFI64:
        return "UEFI";
    case LIMINE_FIRMWARE_TYPE_EFI32:
        return "UEFI (32-bit)";
    default:
        return "unknown";
    }
}

static void log_boot_environment(void)
{
    const struct limine_bootloader_info_response *info = bootloader_info_request.response;
    const struct limine_firmware_type_response *fw = firmware_type_request.response;

    if (info != NULL) {
        klog(LOG_INFO, "bootloader: %s %s", info->name, info->version);
    }
    klog(LOG_INFO, "firmware: %s", fw != NULL ? firmware_name(fw->firmware_type) : "unknown");
}

/*
 * Interim boot indicator until the design system defines the brand (Phase 6):
 * a dark background with a centred square frame. Pure rectangles, so it
 * needs no font or image data.
 */
static void draw_boot_screen(const struct bootfb *fb)
{
    const uint32_t background = bootfb_rgb(fb, 0x12, 0x15, 0x1A);
    const uint32_t accent = bootfb_rgb(fb, 0x4C, 0x8D, 0xF6);
    const uint32_t shorter = fb->width < fb->height ? fb->width : fb->height;
    const uint32_t size = shorter / 6;
    const uint32_t border = size / 6;
    const uint32_t x = (fb->width - size) / 2;
    const uint32_t y = (fb->height - size) / 2;

    bootfb_fill_rect(fb, 0, 0, fb->width, fb->height, background);
    bootfb_fill_rect(fb, x, y, size, size, accent);
    bootfb_fill_rect(fb, x + border, y + border, size - 2 * border, size - 2 * border, background);
}

static void init_boot_screen(void)
{
    const struct limine_framebuffer_response *resp = framebuffer_request.response;
    const struct limine_framebuffer *lfb;
    struct bootfb_format format;
    struct bootfb fb;

    if (resp == NULL || resp->framebuffer_count == 0) {
        klog(LOG_INFO, "framebuffer: none");
        return;
    }
    lfb = resp->framebuffers[0];
    format = (struct bootfb_format){
        .red_shift = lfb->red_mask_shift,
        .red_size = lfb->red_mask_size,
        .green_shift = lfb->green_mask_shift,
        .green_size = lfb->green_mask_size,
        .blue_shift = lfb->blue_mask_shift,
        .blue_size = lfb->blue_mask_size,
    };
    if (lfb->memory_model != LIMINE_FRAMEBUFFER_RGB ||
        !bootfb_init(&fb, lfb->address, lfb->width, lfb->height, lfb->pitch, lfb->bpp, &format)) {
        klog(LOG_WARN, "framebuffer: unsupported format");
        return;
    }
    draw_boot_screen(&fb);
    klog(LOG_INFO, "framebuffer: %ux%u 32bpp", fb.width, fb.height);
}

static const char *kernel_cmdline(void)
{
    const struct limine_executable_cmdline_response *resp = cmdline_request.response;

    return resp != NULL && resp->cmdline != NULL ? resp->cmdline : "";
}

void kmain(void)
{
    const char *cmdline;
    const char *ktest_selection;
    size_t ktest_selection_len;
    const char *loglevel;
    size_t loglevel_len;

    log_init();
    log_write("\n");
    klog(LOG_INFO, "SMY Kernel starting");

    if (!LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision)) {
        panic("bootloader does not support Limine base revision 6");
    }
    arch_cpu_init();
    klog(LOG_INFO, "cpu: GDT, TSS and IDT loaded; exception handlers active");

    cmdline = kernel_cmdline();
    if (cmdline[0] != '\0') {
        klog(LOG_INFO, "command line: %s", cmdline);
    }
    if (cmdline_find(cmdline, "smy.loglevel", &loglevel, &loglevel_len) &&
        cmdline_value_is(loglevel, loglevel_len, "debug")) {
        log_set_level(LOG_DEBUG);
    }
    log_boot_environment();
    init_boot_screen();

    if (cmdline_find(cmdline, "smy.ktest", &ktest_selection, &ktest_selection_len)) {
        panic_set_test_mode(true);
        ktest_run(ktest_selection, ktest_selection_len);
    }

    klog(LOG_INFO, "boot OK");
    arch_halt_forever();
}
