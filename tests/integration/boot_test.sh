#!/bin/sh
# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 The SMY-OS Authors
#
# Boot the SMY-OS ISO in QEMU and wait for "SMY-OS: boot OK" on the serial
# console. Language: POSIX sh (ADR 0002).
#
# Usage: tests/integration/boot_test.sh <bios|uefi> <cd|disk> [iso]
#   cd    the ISO is attached as a CD-ROM
#   disk  the ISO is attached as a raw hard disk (what a USB stick written
#         with dd looks like to the firmware). QEMU runs with -snapshot, so
#         the image file is never modified.
#
# Pass:  the line "SMY-OS: boot OK" appears and the firmware line matches the
#        requested mode.
# Fail:  "SMY-OS: PANIC", QEMU exits early, or BOOT_TIMEOUT seconds pass
#        (a hang is a failure). Default timeout 60 s: QEMU runs under TCG
#        when KVM is unavailable (cloud sessions, GitHub runners).
#
# Safety: only regular image files are accepted, never block devices.

set -eu

firmware=${1:-}
media=${2:-}
iso=${3:-build/smy-os.iso}
timeout_s=${BOOT_TIMEOUT:-60}
ovmf_code=${OVMF_CODE:-/usr/share/OVMF/OVMF_CODE_4M.fd}
ovmf_vars=${OVMF_VARS:-/usr/share/OVMF/OVMF_VARS_4M.fd}

usage() {
    echo "usage: $0 <bios|uefi> <cd|disk> [iso]" >&2
    exit 2
}

case "$firmware" in bios | uefi) ;; *) usage ;; esac
case "$media" in cd | disk) ;; *) usage ;; esac

if [ -b "$iso" ] || [ -c "$iso" ]; then
    echo "boot_test: refusing device node $iso (image files only)" >&2
    exit 2
fi
if [ ! -f "$iso" ]; then
    echo "boot_test: $iso not found (run 'make iso')" >&2
    exit 2
fi

workdir=$(mktemp -d)
log="$workdir/serial.log"
qemu_pid=
cleanup() {
    if [ -n "$qemu_pid" ]; then
        kill "$qemu_pid" 2>/dev/null || true
        wait "$qemu_pid" 2>/dev/null || true
    fi
    rm -rf "$workdir"
}
trap cleanup EXIT INT TERM

set -- -machine q35 -m 256M -smp 1 -display none -no-reboot \
    -serial "file:$log" -monitor none

if [ -r /dev/kvm ] && [ -w /dev/kvm ]; then
    set -- "$@" -accel kvm
else
    set -- "$@" -accel tcg
fi

if [ "$firmware" = uefi ]; then
    # OVMF needs a private, writable copy of its variable store.
    cp "$ovmf_vars" "$workdir/vars.fd"
    set -- "$@" \
        -drive "if=pflash,format=raw,readonly=on,file=$ovmf_code" \
        -drive "if=pflash,format=raw,file=$workdir/vars.fd"
    expect_fw="SMY-OS: firmware: UEFI"
else
    expect_fw="SMY-OS: firmware: BIOS"
fi

if [ "$media" = cd ]; then
    set -- "$@" -drive "file=$iso,media=cdrom,format=raw,readonly=on"
else
    set -- "$@" -drive "file=$iso,media=disk,format=raw,if=ide" -snapshot
fi

echo "boot_test: $firmware/$media, timeout ${timeout_s}s"
start=$(date +%s)
qemu-system-x86_64 "$@" 2>"$workdir/qemu.err" &
qemu_pid=$!

result=timeout
while :; do
    if grep -q "SMY-OS: PANIC" "$log" 2>/dev/null; then
        result=panic
        break
    fi
    if grep -q "SMY-OS: boot OK" "$log" 2>/dev/null; then
        result=ok
        break
    fi
    if ! kill -0 "$qemu_pid" 2>/dev/null; then
        result=qemu-exited
        break
    fi
    if [ $(($(date +%s) - start)) -ge "$timeout_s" ]; then
        break
    fi
    sleep 0.5
done
elapsed=$(($(date +%s) - start))

echo "----- serial log ($firmware/$media) -----"
tr -d '\r' <"$log" 2>/dev/null | grep "SMY-OS" || true
echo "-----------------------------------------"

if [ "$result" = ok ] && ! grep -q "$expect_fw" "$log"; then
    echo "boot_test: FAIL $firmware/$media: expected '$expect_fw'"
    exit 1
fi
if [ "$result" != ok ]; then
    echo "boot_test: FAIL $firmware/$media: $result after ${elapsed}s"
    echo "----- full serial log -----"
    cat "$log" 2>/dev/null || true
    echo "----- qemu stderr -----"
    cat "$workdir/qemu.err" 2>/dev/null || true
    exit 1
fi
echo "boot_test: PASS $firmware/$media in ${elapsed}s"
