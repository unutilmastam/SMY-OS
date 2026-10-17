# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 The SMY-OS Authors
#
# Bootloader build and the hybrid ISO (docs/install/iso-spec.md, ADR 0007,
# ADR 0008). Limine is built from the SHA-256-verified release tarball with
# the project toolchain; the ISO boots from CD and from a raw-written USB
# disk, on UEFI and on BIOS.

LIMINE_VERSION := 12.9.3
LIMINE_SRC     := third_party/downloads/limine-$(LIMINE_VERSION)
LIMINE_BUILD   := $(BUILD)/limine
LIMINE_STAMP   := $(LIMINE_BUILD)/.smy-built
LIMINE_TOOL    := $(LIMINE_BUILD)/bin/limine

# Map absolute source/build paths out of debug info: Limine embeds a
# build-id hash of its BIOS stages, so path-dependent objects would make the
# ISO depend on the build directory (docs/install/iso-spec.md §5).
LIMINE_CFLAGS := -g -O2 -pipe \
	-ffile-prefix-map=$(abspath $(LIMINE_BUILD))/=limine-build/ \
	-ffile-prefix-map=$(abspath $(LIMINE_SRC))/=limine-src/

LIMINE_CONFIGURE_FLAGS := \
	CC=$(CC) CC_FOR_TARGET=$(CC) LD_FOR_TARGET=$(LD) \
	CFLAGS_FOR_TARGET="$(LIMINE_CFLAGS)" \
	OBJCOPY_FOR_TARGET=llvm-objcopy OBJDUMP_FOR_TARGET=llvm-objdump \
	READELF_FOR_TARGET=llvm-readelf \
	--enable-bios --enable-bios-cd --enable-uefi-x86-64 --enable-uefi-cd

ISO_ROOT := $(BUILD)/iso_root
ISO      := $(BUILD)/smy-os.iso
ISO_SUMS := $(BUILD)/SHA256SUMS

.PHONY: limine iso

limine: $(LIMINE_STAMP)

# fetch-limine.sh verifies the tarball hash and unpacks a fresh tree.
$(LIMINE_STAMP): tools/fetch-limine.sh boot/boot.mk
	$(Q)tools/fetch-limine.sh
	@echo "  LIMINE  configure + build $(LIMINE_VERSION)"
	$(Q)rm -rf $(LIMINE_BUILD) && mkdir -p $(LIMINE_BUILD)
	$(Q)cd $(LIMINE_BUILD) && $(CURDIR)/$(LIMINE_SRC)/configure $(LIMINE_CONFIGURE_FLAGS) \
		> configure.log 2>&1 || { cat configure.log; exit 1; }
	$(Q)$(MAKE) -C $(LIMINE_BUILD) > $(LIMINE_BUILD)/build.log 2>&1 \
		|| { tail -n 50 $(LIMINE_BUILD)/build.log; exit 1; }
	$(Q)touch $@

iso: $(ISO)

# Layout: docs/install/iso-spec.md §2. Every file gets the same timestamp
# (SOURCE_DATE_EPOCH); xorriso also reads SOURCE_DATE_EPOCH for volume dates
# and the volume UUID, so the ISO is byte-for-byte reproducible.
$(ISO): $(KERNEL_ELF) $(LIMINE_STAMP) boot/limine.conf LICENSE NOTICE
	@echo "  ISO     $@"
	$(Q)rm -rf $(ISO_ROOT) && mkdir -p $(ISO_ROOT)/boot/limine $(ISO_ROOT)/EFI/BOOT
	$(Q)cp $(KERNEL_ELF) $(ISO_ROOT)/boot/smy-kernel.elf
	$(Q)cp boot/limine.conf $(ISO_ROOT)/boot/limine/limine.conf
	$(Q)cp $(LIMINE_BUILD)/bin/limine-bios.sys $(LIMINE_BUILD)/bin/limine-bios-cd.bin \
		$(LIMINE_BUILD)/bin/limine-uefi-cd.bin $(ISO_ROOT)/boot/limine/
	$(Q)cp $(LIMINE_BUILD)/bin/BOOTX64.EFI $(ISO_ROOT)/EFI/BOOT/BOOTX64.EFI
	$(Q)cp $(LIMINE_SRC)/COPYING $(ISO_ROOT)/boot/limine/COPYING
	$(Q)cp $(LIMINE_SRC)/3RDPARTY.md $(ISO_ROOT)/boot/limine/3RDPARTY.md
	$(Q)cp LICENSE NOTICE $(ISO_ROOT)/
	$(Q)find $(ISO_ROOT) -exec touch -h -d @$(SOURCE_DATE_EPOCH) {} +
	$(Q)xorriso -as mkisofs -R -r -J -V SMY-OS \
		-b boot/limine/limine-bios-cd.bin -no-emul-boot -boot-load-size 4 \
		-boot-info-table -hfsplus -apm-block-size 2048 \
		--efi-boot boot/limine/limine-uefi-cd.bin -efi-boot-part --efi-boot-image \
		--protective-msdos-label $(ISO_ROOT) -o $@.tmp > $(BUILD)/xorriso.log 2>&1 \
		|| { cat $(BUILD)/xorriso.log; exit 1; }
	$(Q)$(LIMINE_TOOL) bios-install $@.tmp > $(BUILD)/limine-bios-install.log 2>&1 \
		|| { cat $(BUILD)/limine-bios-install.log; exit 1; }
	$(Q)mv $@.tmp $@
	$(Q)cd $(BUILD) && sha256sum smy-os.iso > SHA256SUMS
	@cat $(ISO_SUMS)
