# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 The SMY-OS Authors
#
# QEMU integration tests (TESTING.md). Every combination of firmware
# (BIOS, UEFI) and medium (CD, raw disk = USB stick) must reach
# "SMY-OS: boot OK".

BOOT_TEST := tests/integration/boot_test.sh

.PHONY: boot-test boot-test-bios-cd boot-test-bios-disk boot-test-uefi-cd boot-test-uefi-disk run \
	boot-screen boot-screen-bios boot-screen-uefi

boot-test: boot-test-bios-cd boot-test-bios-disk boot-test-uefi-cd boot-test-uefi-disk

boot-test-bios-cd: $(ISO)
	$(Q)$(BOOT_TEST) bios cd $(ISO)

boot-test-bios-disk: $(ISO)
	$(Q)$(BOOT_TEST) bios disk $(ISO)

boot-test-uefi-cd: $(ISO)
	$(Q)$(BOOT_TEST) uefi cd $(ISO)

boot-test-uefi-disk: $(ISO)
	$(Q)$(BOOT_TEST) uefi disk $(ISO)

# Boot screen (brand logo on VOID) checked pixel by pixel on both firmwares;
# the screenshots stay in build/screens/ for review (CI uploads them).
CHECK_SCREEN := tests/integration/check_boot_screen.py

boot-screen: boot-screen-bios boot-screen-uefi

boot-screen-bios boot-screen-uefi: boot-screen-%: $(ISO)
	@mkdir -p $(BUILD)/screens
	$(Q)BOOT_SCREENSHOT=$(BUILD)/screens/boot-$*.ppm $(BOOT_TEST) $* cd $(ISO)
	$(Q)python3 $(CHECK_SCREEN) $(BUILD)/screens/boot-$*.ppm
	$(Q)convert $(BUILD)/screens/boot-$*.ppm $(BUILD)/screens/boot-$*.png

# Interactive run: serial on the terminal, no graphics window
# (add QEMU_EXTRA="-s -S" to wait for GDB, see BUILDING.md).
run: $(ISO)
	qemu-system-x86_64 -machine q35 -m 256M -serial stdio -display none \
		-drive file=$(ISO),media=cdrom,format=raw,readonly=on $(QEMU_EXTRA)

# In-kernel self-tests (tests/integration/ktest.sh, docs/kernel/ktest.md):
# the full suite on both firmwares, and each panic path with the exact
# report lines it must produce.
KTEST_SH := tests/integration/ktest.sh

.PHONY: ktest ktest-bios ktest-uefi ktest-big-memory ktest-cpu-max ktest-no-hpet \
	ktest-keyboard-bios ktest-keyboard-uefi ktest-panic-ud \
	ktest-panic-pf ktest-panic-kernel ktest-panic-stack-overflow ktest-panic-double-kfree

ktest: ktest-bios ktest-uefi ktest-big-memory ktest-cpu-max ktest-no-hpet ktest-keyboard-bios \
	ktest-keyboard-uefi ktest-panic-ud ktest-panic-pf \
	ktest-panic-kernel ktest-panic-stack-overflow ktest-panic-double-kfree

ktest-bios: $(ISO)
	$(Q)ISO=$(ISO) $(KTEST_SH) bios all pass "SMY-OS: ktest: .* 0 failed"

ktest-uefi: $(ISO)
	$(Q)ISO=$(ISO) $(KTEST_SH) uefi all pass "SMY-OS: ktest: .* 0 failed"

# 4 GiB of RAM puts memory above the 4 GiB boundary and a PCI hole into the
# map; the allocator test takes every page (about a million) and returns it.
ktest-big-memory: $(ISO)
	$(Q)ISO=$(ISO) KTEST_MEMORY=4G KTEST_TIMEOUT=240 $(KTEST_SH) uefi \
		phys_allocates_all_memory_and_frees_without_leak pass "SMY-OS: memory: 40[0-9][0-9] MiB usable"

# A CPU model with SMEP and SMAP: the protections must be switched on and
# the whole suite must still pass with them.
ktest-cpu-max: $(ISO)
	$(Q)ISO=$(ISO) KTEST_CPU=max $(KTEST_SH) uefi all pass "SMY-OS: ktest: .* 0 failed" \
		"SMY-OS: vmm: .*NX on, WP on, SMEP on, SMAP on"

# Without an HPET the local APIC timer is calibrated with the PIT.
ktest-no-hpet: $(ISO)
	$(Q)ISO=$(ISO) KTEST_MACHINE=q35,hpet=off $(KTEST_SH) bios all pass \
		"SMY-OS: ktest: .* 0 failed" "SMY-OS: platform: .*calibrated with PIT"

# Keystrokes typed through the QEMU monitor reach the PS/2 driver as HID
# key events (kernel/arch/x86_64/ktest_platform.c).
ktest-keyboard-bios ktest-keyboard-uefi: ktest-keyboard-%: $(ISO)
	$(Q)ISO=$(ISO) KTEST_SENDKEYS="s m y up ret" $(KTEST_SH) $* keyboard_receives_injected_keys pass \
		"SMY-OS: ktest:   key 0x52 down" "SMY-OS: ktest: 1 run, 1 passed"

ktest-panic-ud: $(ISO)
	$(Q)ISO=$(ISO) $(KTEST_SH) bios panic_on_unhandled_invalid_opcode panic \
		"SMY-OS: PANIC: unhandled CPU exception #UD (invalid opcode), vector 6" \
		"SMY-OS:   RIP=ffffffff8" "SMY-OS:   CR0=" "SMY-OS: stack trace:" "SMY-OS:   #1  0x"

ktest-panic-pf: $(ISO)
	$(Q)ISO=$(ISO) $(KTEST_SH) uefi panic_on_unhandled_page_fault panic \
		"SMY-OS: PANIC: unhandled CPU exception #PF (page fault), vector 14" \
		"fault address (CR2) 0xffffffff90000000: page not present, read, kernel mode" \
		"SMY-OS:   page walk: not present at PDE"

ktest-panic-kernel: $(ISO)
	$(Q)ISO=$(ISO) $(KTEST_SH) bios panic_from_kernel_code panic \
		"SMY-OS: PANIC: ktest: deliberate panic with value 42" "SMY-OS:   #0  0x"

# Endless recursion on the guard-paged main stack: the push into the guard
# page cannot deliver its #PF, so the CPU raises #DF on its own IST stack.
ktest-panic-stack-overflow: $(ISO)
	$(Q)ISO=$(ISO) $(KTEST_SH) bios panic_on_kernel_stack_overflow panic \
		"SMY-OS: PANIC: unhandled CPU exception #DF (double fault), vector 8" \
		"SMY-OS:   kernel stack overflow: guard page below the stack" "SMY-OS:   #1  0x"

ktest-panic-double-kfree: $(ISO)
	$(Q)ISO=$(ISO) $(KTEST_SH) uefi panic_on_double_kfree panic \
		"SMY-OS: PANIC: kfree(0xffff8.*): double free"
