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
#include "drivers/bootfb/boot_logo.h"
#include "drivers/bootfb/bootfb.h"
#include "memory/kmalloc.h"
#include "memory/phys.h"
#include "memory/vmm.h"

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

SMY_USED SMY_SECTION(".limine_requests") static volatile struct limine_memmap_request
    memmap_request = {.id = LIMINE_MEMMAP_REQUEST_ID, .revision = 0};

SMY_USED SMY_SECTION(".limine_requests") static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST_ID, .revision = 0};

SMY_USED SMY_SECTION(".limine_requests") static volatile struct limine_executable_address_request
    executable_address_request = {.id = LIMINE_EXECUTABLE_ADDRESS_REQUEST_ID, .revision = 0};

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
 * Boot screen (docs/BRAND_PROMPT.md, design/brand/logos/smy-boot-logo.svg):
 * the VOID background with the SMY symbol centred. The logo is the largest
 * pre-rendered size that is at most a fifth of the shorter screen side; on
 * a screen too small for any of them only the background is drawn.
 */
static void draw_boot_screen(const struct bootfb *fb)
{
    const uint32_t shorter = fb->width < fb->height ? fb->width : fb->height;
    const struct boot_logo *logo = NULL;

    bootfb_fill_rect(fb, 0, 0, fb->width, fb->height, bootfb_rgb(fb, 0x08, 0x0B, 0x12));
    for (unsigned i = 0; i < boot_logo_count; i++) {
        if (boot_logos[i].width <= shorter / 5) {
            logo = &boot_logos[i];
        }
    }
    if (logo == NULL) {
        return;
    }
    if (!bootfb_blit_rle(fb, (fb->width - logo->width) / 2, (fb->height - logo->height) / 2,
                         logo->width, logo->height, logo->rle, logo->rle_size)) {
        klog(LOG_WARN, "framebuffer: boot logo data invalid");
    }
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

static struct vmm_kernel_image kernel_image(void)
{
    const struct limine_executable_address_response *exe = executable_address_request.response;

    if (exe == NULL) {
        panic("bootloader did not report where the kernel is loaded");
        return (struct vmm_kernel_image){0}; /* not reached; for cppcheck */
    }
    return (struct vmm_kernel_image){.phys_base = exe->physical_base,
                                     .virt_base = exe->virtual_base};
}

/* Boot stack for everything after the switch to the kernel's own page
 * tables: 64 KiB with a guard page below (memory/vmm.h). */
#define KERNEL_MAIN_STACK_PAGES 16

static const char *boot_cmdline = "";

SMY_NORETURN static void kmain_late(void)
{
    const char *ktest_selection;
    size_t ktest_selection_len;

    arch_cpu_init_late();
    kmalloc_init();
    klog(LOG_INFO, "cpu: exception stacks moved to guard-paged stacks; kernel heap ready");

    init_boot_screen();

    if (cmdline_find(boot_cmdline, "smy.ktest", &ktest_selection, &ktest_selection_len)) {
        panic_set_test_mode(true);
        ktest_run(ktest_selection, ktest_selection_len);
    }

    klog(LOG_INFO, "boot OK");
    arch_halt_forever();
}

void kmain(void)
{
    const char *cmdline;
    const char *loglevel;
    size_t loglevel_len;
    struct vmm_kernel_image image;

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

    if (hhdm_request.response == NULL) {
        panic("no higher-half direct map from the bootloader");
    }
    phys_init(memmap_request.response, hhdm_request.response->offset);

    image = kernel_image();
    vmm_init(memmap_request.response, hhdm_request.response->offset, &image);

    /* The command line string lives in bootloader-reclaimable memory, which
     * the direct map keeps mapped. */
    boot_cmdline = cmdline;
    arch_switch_stack(vmm_alloc_stack(KERNEL_MAIN_STACK_PAGES), kmain_late);
}
