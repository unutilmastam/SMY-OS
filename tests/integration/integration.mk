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

# In-kernel self-tests (tests/integration/ktest.sh, docs/kernel/ktest.md):
# the full suite on both firmwares, and each panic path with the exact
# report lines it must produce.
KTEST_SH := tests/integration/ktest.sh

.PHONY: ktest ktest-bios ktest-uefi ktest-panic-ud ktest-panic-pf ktest-panic-kernel

ktest: ktest-bios ktest-uefi ktest-panic-ud ktest-panic-pf ktest-panic-kernel

ktest-bios: $(ISO)
	$(Q)ISO=$(ISO) $(KTEST_SH) bios all pass "SMY-OS: ktest: .* 0 failed"

ktest-uefi: $(ISO)
	$(Q)ISO=$(ISO) $(KTEST_SH) uefi all pass "SMY-OS: ktest: .* 0 failed"

ktest-panic-ud: $(ISO)
	$(Q)ISO=$(ISO) $(KTEST_SH) bios panic_on_unhandled_invalid_opcode panic \
		"SMY-OS: PANIC: unhandled CPU exception #UD (invalid opcode), vector 6" \
		"SMY-OS:   RIP=ffffffff8" "SMY-OS:   CR0=" "SMY-OS: stack trace:" "SMY-OS:   #1  0x"

ktest-panic-pf: $(ISO)
	$(Q)ISO=$(ISO) $(KTEST_SH) uefi panic_on_unhandled_page_fault panic \
		"SMY-OS: PANIC: unhandled CPU exception #PF (page fault), vector 14" \
		"fault address (CR2) 0xffffffff90000000: page not present, read, kernel mode"

ktest-panic-kernel: $(ISO)
	$(Q)ISO=$(ISO) $(KTEST_SH) bios panic_from_kernel_code panic \
		"SMY-OS: PANIC: ktest: deliberate panic with value 42" "SMY-OS:   #0  0x"
