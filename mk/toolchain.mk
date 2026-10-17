# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 The SMY-OS Authors
#
# Toolchain and flags (ADR 0002, ADR 0003). All tools are overridable from the
# command line, e.g. `make CC=clang-18`. The warning list must match
# CONTRIBUTING.md §4.

CC           ?= clang
AR           := llvm-ar
LD           := ld.lld
OBJCOPY      := llvm-objcopy
CLANG_FORMAT ?= clang-format
CLANG_TIDY   ?= clang-tidy
CPPCHECK     ?= cppcheck

# Reproducible builds: one timestamp for every generated artefact (ISO
# volume dates, file dates). Defaults to the last commit time.
SOURCE_DATE_EPOCH ?= $(shell git log -1 --format=%ct 2>/dev/null || echo 0)
export SOURCE_DATE_EPOCH

# `make` sets CC=cc by default; the project requires clang.
ifeq ($(origin CC),default)
CC := clang
endif

WARN_FLAGS := -Wall -Wextra -Werror -Wpedantic -Wshadow -Wconversion \
	-Wsign-conversion -Wstrict-prototypes -Wmissing-prototypes -Wvla \
	-Wcast-align -Wnull-dereference -Wdouble-promotion -Wformat=2

# -ffile-prefix-map keeps absolute build paths out of debug info, so the
# kernel ELF (and the ISO) do not depend on where the tree is checked out.
COMMON_CFLAGS := -std=c17 $(WARN_FLAGS) -MMD -MP -ffile-prefix-map=$(CURDIR)/=

# Kernel code: bare-metal x86-64, higher-half (-mcmodel=kernel), no red zone
# (interrupts would clobber it), no FPU/SIMD state, frame pointers kept for
# stack traces. -ffreestanding also stops clang from turning loops in the
# kernel's own memcpy/memset into calls to themselves.
KERNEL_TARGET := x86_64-unknown-none-elf
KERNEL_CFLAGS := --target=$(KERNEL_TARGET) $(COMMON_CFLAGS) -O2 -g \
	-ffreestanding -nostdlib -fno-builtin -fno-pic -fno-pie \
	-mcmodel=kernel -mno-red-zone \
	-mno-sse -mno-sse2 -mno-mmx -mno-80387 \
	-fno-omit-frame-pointer -fstack-protector-strong \
	-fno-asynchronous-unwind-tables -fno-lto

# Host unit tests: same warnings, sanitizers on, debug-friendly.
HOST_SAN_FLAGS := -fsanitize=address,undefined -fno-sanitize-recover=all
HOST_CFLAGS    := $(COMMON_CFLAGS) -O1 -g -fno-omit-frame-pointer $(HOST_SAN_FLAGS)
HOST_LDFLAGS   := $(HOST_SAN_FLAGS)
