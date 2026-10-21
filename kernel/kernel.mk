# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 The SMY-OS Authors
#
# SMY Kernel: compile C and assembly with kernel flags, link the higher-half
# ELF that Limine loads (kernel/arch/x86_64/linker.ld).

KERNEL_C_SOURCES := \
	kernel/arch/x86_64/cpu.c \
	kernel/arch/x86_64/descriptors.c \
	kernel/arch/x86_64/exceptions.c \
	kernel/arch/x86_64/gdt.c \
	kernel/arch/x86_64/idt.c \
	kernel/arch/x86_64/ktest_cpu.c \
	kernel/core/cmdline.c \
	kernel/core/fmt.c \
	kernel/core/ktest.c \
	kernel/core/ktest_core.c \
	kernel/core/log.c \
	kernel/core/main.c \
	kernel/core/panic.c \
	kernel/core/stack_protector.c \
	kernel/core/stacktrace.c \
	kernel/core/string.c \
	kernel/drivers/bootfb/bootfb.c \
	kernel/drivers/serial/uart16550.c \
	kernel/memory/ktest_phys.c \
	kernel/memory/phys.c \
	kernel/memory/pmm.c

KERNEL_ASM_SOURCES := \
	kernel/arch/x86_64/cpu_asm.S \
	kernel/arch/x86_64/entry.S \
	kernel/arch/x86_64/isr.S \
	kernel/arch/x86_64/ktest_triggers.S

KERNEL_LINKER_SCRIPT := kernel/arch/x86_64/linker.ld

KERNEL_INCLUDES := -Ikernel -Ithird_party/limine-protocol/include

# Boot logo pixels, rendered from the brand SVG at build time
# (tools/design/boot_logo.py): background VOID, sizes for small and large
# screens. Needs rsvg-convert and ImageMagick (tools/check-env.sh).
BOOT_LOGO_SVG     := design/brand/logos/smy-boot-logo.svg
BOOT_LOGO_C       := $(BUILD)/generated/boot_logo.c
BOOT_LOGO_OBJ     := $(BUILD)/generated/boot_logo.o
BOOT_LOGO_BG      := \#080B12
BOOT_LOGO_SIZES   := 96 128 192 256

KERNEL_OBJS := $(KERNEL_C_SOURCES:%.c=$(BUILD)/%.o) $(KERNEL_ASM_SOURCES:%.S=$(BUILD)/%.o) \
	$(BOOT_LOGO_OBJ)
KERNEL_ELF  := $(BUILD)/kernel/smy-kernel.elf

KERNEL_LDFLAGS := -nostdlib -static -z max-page-size=0x1000 -z noexecstack \
	--no-dynamic-linker --build-id=none -T $(KERNEL_LINKER_SCRIPT)

KERNEL_TIDY_FLAGS := $(filter-out -MMD -MP -Werror,$(KERNEL_CFLAGS)) $(KERNEL_INCLUDES)

.PHONY: kernel

kernel: $(KERNEL_ELF)

$(KERNEL_ELF): $(KERNEL_OBJS) $(KERNEL_LINKER_SCRIPT)
	@mkdir -p $(@D)
	@echo "  LD      $@"
	$(Q)$(LD) $(KERNEL_LDFLAGS) $(KERNEL_OBJS) -o $@

$(BUILD)/kernel/%.o: kernel/%.c mk/toolchain.mk kernel/kernel.mk
	@mkdir -p $(@D)
	@echo "  CC      $<"
	$(Q)$(CC) $(KERNEL_CFLAGS) $(KERNEL_INCLUDES) -c $< -o $@

$(BUILD)/kernel/%.o: kernel/%.S mk/toolchain.mk kernel/kernel.mk
	@mkdir -p $(@D)
	@echo "  AS      $<"
	$(Q)$(CC) $(KERNEL_CFLAGS) $(KERNEL_INCLUDES) -c $< -o $@

$(BOOT_LOGO_C): $(BOOT_LOGO_SVG) tools/design/boot_logo.py kernel/kernel.mk
	@mkdir -p $(@D)
	@echo "  LOGO    $@"
	$(Q)python3 tools/design/boot_logo.py $(BOOT_LOGO_SVG) $@ '$(BOOT_LOGO_BG)' $(BOOT_LOGO_SIZES)

# The build directory can be anywhere (tools/check-reproducible.sh), so its
# path is mapped away like the source path.
$(BOOT_LOGO_OBJ): $(BOOT_LOGO_C) kernel/drivers/bootfb/boot_logo.h mk/toolchain.mk kernel/kernel.mk
	@echo "  CC      $<"
	$(Q)$(CC) $(KERNEL_CFLAGS) $(KERNEL_INCLUDES) \
		-ffile-prefix-map=$(BUILD)/generated=generated \
		-ffile-prefix-map=$(abspath $(BUILD))/generated=generated -c $< -o $@

-include $(KERNEL_OBJS:.o=.d)
