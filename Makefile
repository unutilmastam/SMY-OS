# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 The SMY-OS Authors
#
# SMY-OS top-level build (ADR 0004): non-recursive GNU Make. Each directory
# contributes a *.mk fragment; all outputs go to $(BUILD) (git-ignored).

.DEFAULT_GOAL := all
.DELETE_ON_ERROR:
.SUFFIXES:

BUILD ?= build

ifeq ($(V),1)
Q :=
else
Q := @
endif

include mk/toolchain.mk
include kernel/kernel.mk
include boot/boot.mk
include tests/unit/unit.mk
include tests/integration/integration.mk

# Every C source that format-check and lint look at.
ALL_C_SOURCES := $(KERNEL_C_SOURCES) $(UNIT_C_SOURCES)
ALL_C_HEADERS := $(shell find kernel tests -name '*.h' | LC_ALL=C sort)

.PHONY: all test check format format-check lint lint-tidy lint-cppcheck clean

all: kernel

test: unit-test

format:
	$(Q)$(CLANG_FORMAT) -i $(ALL_C_SOURCES) $(ALL_C_HEADERS)

format-check:
	$(Q)$(CLANG_FORMAT) --dry-run --Werror $(ALL_C_SOURCES) $(ALL_C_HEADERS)
	@echo "format-check: OK ($(words $(ALL_C_SOURCES) $(ALL_C_HEADERS)) files)"

lint: lint-tidy lint-cppcheck

# clang-tidy runs each source with the flags of the build it belongs to.
lint-tidy:
	$(Q)$(CLANG_TIDY) --quiet $(KERNEL_C_SOURCES) -- $(KERNEL_TIDY_FLAGS)
	$(Q)$(CLANG_TIDY) --quiet $(UNIT_C_SOURCES) -- $(UNIT_TIDY_FLAGS)
	@echo "lint-tidy: OK"

lint-cppcheck:
	$(Q)$(CPPCHECK) --quiet --error-exitcode=1 --std=c11 --inline-suppr \
		--enable=warning,style,performance,portability \
		--suppress=missingIncludeSystem \
		-I kernel -I tests/unit -I third_party/limine-protocol/include \
		$(ALL_C_SOURCES)
	@echo "lint-cppcheck: OK"

# The full sequence CI runs (CONTRIBUTING.md §5).
check: all test format-check lint iso boot-test

clean:
	rm -rf $(BUILD)
