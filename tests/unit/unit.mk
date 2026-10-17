# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 The SMY-OS Authors
#
# Host unit tests (ASan + UBSan). Kernel sources under test are compiled for
# the host with SMY_HOST_TEST, which renames their public symbols
# (kernel/core/string.h). -fno-builtin keeps clang from replacing the code
# under test with calls into the host C library.

UNIT_HARNESS_SOURCES := tests/unit/harness/smytest.c
UNIT_TEST_SOURCES    := \
	tests/unit/kernel/core/test_fmt.c \
	tests/unit/kernel/core/test_string.c \
	tests/unit/kernel/drivers/test_bootfb.c
UNIT_KERNEL_SOURCES  := \
	kernel/core/fmt.c \
	kernel/core/string.c \
	kernel/drivers/bootfb/bootfb.c

UNIT_C_SOURCES := $(UNIT_HARNESS_SOURCES) $(UNIT_TEST_SOURCES)

UNIT_INCLUDES := -Ikernel -Itests/unit
UNIT_DEFINES  := -DSMY_HOST_TEST

UNIT_OBJS := $(UNIT_C_SOURCES:%.c=$(BUILD)/host/%.o) \
	$(UNIT_KERNEL_SOURCES:%.c=$(BUILD)/host/%.o)
UNIT_BIN  := $(BUILD)/host/tests/unit/smy-unit-tests

UNIT_TIDY_FLAGS := $(filter-out -MMD -MP -Werror $(HOST_SAN_FLAGS),$(HOST_CFLAGS)) \
	$(UNIT_INCLUDES) $(UNIT_DEFINES)

.PHONY: unit-test

unit-test: $(UNIT_BIN)
	@echo "  TEST    $<"
	$(Q)$(UNIT_BIN)

$(UNIT_BIN): $(UNIT_OBJS)
	@mkdir -p $(@D)
	@echo "  LD      $@"
	$(Q)$(CC) $(HOST_LDFLAGS) $^ -o $@

$(BUILD)/host/kernel/%.o: kernel/%.c
	@mkdir -p $(@D)
	@echo "  HOSTCC  $<"
	$(Q)$(CC) $(HOST_CFLAGS) -fno-builtin $(UNIT_INCLUDES) $(UNIT_DEFINES) -c $< -o $@

$(BUILD)/host/tests/%.o: tests/%.c
	@mkdir -p $(@D)
	@echo "  HOSTCC  $<"
	$(Q)$(CC) $(HOST_CFLAGS) $(UNIT_INCLUDES) $(UNIT_DEFINES) -c $< -o $@

-include $(UNIT_OBJS:.o=.d)
