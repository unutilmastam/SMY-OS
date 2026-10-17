# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 The SMY-OS Authors
#
# QEMU integration tests (TESTING.md). Every combination of firmware
# (BIOS, UEFI) and medium (CD, raw disk = USB stick) must reach
# "SMY-OS: boot OK".

BOOT_TEST := tests/integration/boot_test.sh

.PHONY: boot-test boot-test-bios-cd boot-test-bios-disk boot-test-uefi-cd boot-test-uefi-disk run

boot-test: boot-test-bios-cd boot-test-bios-disk boot-test-uefi-cd boot-test-uefi-disk

boot-test-bios-cd: $(ISO)
	$(Q)$(BOOT_TEST) bios cd $(ISO)

boot-test-bios-disk: $(ISO)
	$(Q)$(BOOT_TEST) bios disk $(ISO)

boot-test-uefi-cd: $(ISO)
	$(Q)$(BOOT_TEST) uefi cd $(ISO)

boot-test-uefi-disk: $(ISO)
	$(Q)$(BOOT_TEST) uefi disk $(ISO)

# Interactive run: serial on the terminal, no graphics window
# (add QEMU_EXTRA="-s -S" to wait for GDB, see BUILDING.md).
run: $(ISO)
	qemu-system-x86_64 -machine q35 -m 256M -serial stdio -display none \
		-drive file=$(ISO),media=cdrom,format=raw,readonly=on $(QEMU_EXTRA)
