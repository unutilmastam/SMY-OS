# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 The SMY-OS Authors
#
# SMY Kernel. Phase 1 compiles the kernel library objects with kernel flags
# into an archive; linking a bootable kernel ELF starts in Phase 2.

KERNEL_C_SOURCES := kernel/core/string.c

KERNEL_OBJS := $(KERNEL_C_SOURCES:%.c=$(BUILD)/%.o)
KERNEL_LIB  := $(BUILD)/kernel/libsmy-kernel.a

KERNEL_TIDY_FLAGS := $(filter-out -MMD -MP -Werror,$(KERNEL_CFLAGS)) -Ikernel

kernel: $(KERNEL_LIB)

$(KERNEL_LIB): $(KERNEL_OBJS)
	@mkdir -p $(@D)
	@echo "  AR      $@"
	$(Q)rm -f $@
	$(Q)$(AR) rcsD $@ $^

$(BUILD)/kernel/%.o: kernel/%.c
	@mkdir -p $(@D)
	@echo "  CC      $<"
	$(Q)$(CC) $(KERNEL_CFLAGS) -Ikernel -c $< -o $@

-include $(KERNEL_OBJS:.o=.d)
