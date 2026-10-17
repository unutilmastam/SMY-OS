#!/bin/sh
# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 The SMY-OS Authors
#
# Run in-kernel self-tests (ktest) in QEMU. Language: POSIX sh (ADR 0002).
#
# Usage: tests/integration/ktest.sh <bios|uefi> <selection> <pass|panic> [grep-pattern...]
#   selection  value for smy.ktest= ("all" or one test name)
#   pass       expect every selected test to pass (QEMU exit status 33)
#   panic      expect the run to end in a kernel panic (exit status 35 and
#              a "SMY-OS: PANIC:" line)
#   pattern    each extra argument is a basic regular expression that must
#              match at least one line of the serial log
#
# The same ISO as the boot test is used: the kernel command line is handed to
# Limine as a configuration in an SMBIOS OEM string (Limine CONFIG.md,
# "Config via SMBIOS"), so no separate test image is built. The kernel path
# uses fslabel(SMY-OS) because boot() is undefined for SMBIOS configs on BIOS.
#
# Environment: ISO (default build/smy-os.iso), KTEST_TIMEOUT (default 120 s),
# KTEST_MEMORY (QEMU -m value, default 256M).

set -eu

firmware=${1:-}
selection=${2:-}
expect=${3:-}
[ $# -ge 3 ] && shift 3
iso=${ISO:-build/smy-os.iso}
timeout_s=${KTEST_TIMEOUT:-120}
memory=${KTEST_MEMORY:-256M}
ovmf_code=${OVMF_CODE:-/usr/share/OVMF/OVMF_CODE_4M.fd}
ovmf_vars=${OVMF_VARS:-/usr/share/OVMF/OVMF_VARS_4M.fd}

usage() {
    echo "usage: $0 <bios|uefi> <selection> <pass|panic> [pattern...]" >&2
    exit 2
}

case "$firmware" in bios | uefi) ;; *) usage ;; esac
case "$expect" in pass | panic) ;; *) usage ;; esac
case "$selection" in "" | *[!A-Za-z0-9_]*) usage ;; esac

if [ -b "$iso" ] || [ -c "$iso" ] || [ ! -f "$iso" ]; then
    echo "ktest: $iso is not a regular image file (run 'make iso')" >&2
    exit 2
fi

workdir=$(mktemp -d)
log="$workdir/serial.log"
trap 'rm -rf "$workdir"' EXIT INT TERM

config=$(printf 'limine:config:timeout: 0\nserial: yes\n/SMY-OS ktest\n    protocol: limine\n    path: fslabel(SMY-OS):/boot/smy-kernel.elf\n    cmdline: smy.ktest=%s' "$selection")

if [ -r /dev/kvm ] && [ -w /dev/kvm ]; then
    accel=kvm
else
    accel=tcg
fi

# run_qemu [extra QEMU arguments...]
run_qemu() {
    timeout "$timeout_s" qemu-system-x86_64 -accel "$accel" \
        -machine q35 -m "$memory" -smp 1 -display none -no-reboot -monitor none \
        -serial "file:$log" \
        -device isa-debug-exit,iobase=0xf4,iosize=0x04 \
        -smbios "type=11,value=$config" \
        -drive "file=$iso,media=cdrom,format=raw,readonly=on" \
        "$@" 2>"$workdir/qemu.err"
}

echo "ktest: $firmware, smy.ktest=$selection, expect $expect, memory $memory, timeout ${timeout_s}s"
start=$(date +%s)
status=0
if [ "$firmware" = uefi ]; then
    # OVMF needs a private, writable copy of its variable store.
    cp "$ovmf_vars" "$workdir/vars.fd"
    run_qemu -drive "if=pflash,format=raw,readonly=on,file=$ovmf_code" \
        -drive "if=pflash,format=raw,file=$workdir/vars.fd" || status=$?
else
    run_qemu || status=$?
fi
elapsed=$(($(date +%s) - start))

echo "----- serial log (SMY-OS lines) -----"
tr -d '\r' <"$log" | grep "SMY-OS" || true
echo "-------------------------------------"

fail() {
    echo "ktest: FAIL ($firmware, $selection): $1 after ${elapsed}s"
    echo "----- qemu stderr -----"
    cat "$workdir/qemu.err" || true
    exit 1
}

case "$status" in
33) outcome=pass ;;
35) outcome=fail ;;
124) fail "timeout (hang)" ;;
*) fail "QEMU exited with status $status (no ktest result)" ;;
esac

if [ "$expect" = pass ] && [ "$outcome" != pass ]; then
    fail "tests failed (exit status $status)"
fi
if [ "$expect" = panic ]; then
    [ "$outcome" = fail ] || fail "expected a panic, but the run reported success"
    grep -q "SMY-OS: PANIC:" "$log" || fail "no 'SMY-OS: PANIC:' line"
fi

for pattern in "$@"; do
    if ! tr -d '\r' <"$log" | grep -q -- "$pattern"; then
        fail "serial log has no line matching: $pattern"
    fi
done

echo "ktest: PASS ($firmware, $selection, $expect) in ${elapsed}s"
