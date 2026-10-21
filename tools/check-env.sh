#!/bin/sh
# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 The SMY-OS Authors
#
# Verify the SMY-OS development tools and print their versions.
# Language: POSIX sh (ADR 0002).
#
# Usage: tools/check-env.sh [--all]
#   default  fail if a tool needed for kernel build, unit tests and lint is
#            missing; report ISO/boot-test tools (QEMU, OVMF, xorriso,
#            mtools, nasm) as WARN.
#   --all    also fail if an ISO/boot-test tool is missing (CI uses this).
#
# Install everything on Ubuntu 24.04: see BUILDING.md §2.

set -eu

MIN_LLVM_MAJOR=18
strict_all=0
[ "${1:-}" = "--all" ] && strict_all=1

errors=0
warnings=0

first_line() {
    "$@" 2>&1 | head -n 1
}

# need <name> <command> <version-command...>
need() {
    name=$1
    cmd=$2
    shift 2
    if command -v "$cmd" >/dev/null 2>&1; then
        printf '  OK    %-20s %s\n' "$name" "$(first_line "$@")"
    else
        printf '  MISS  %-20s (command "%s" not found)\n' "$name" "$cmd"
        errors=$((errors + 1))
    fi
}

# later <name> <command> <version-command...>: needed for make iso / boot-test.
later() {
    name=$1
    cmd=$2
    shift 2
    if command -v "$cmd" >/dev/null 2>&1; then
        printf '  OK    %-20s %s\n' "$name" "$(first_line "$@")"
    elif [ "$strict_all" -eq 1 ]; then
        printf '  MISS  %-20s (command "%s" not found)\n' "$name" "$cmd"
        errors=$((errors + 1))
    else
        printf '  WARN  %-20s (not found; needed for make iso / boot-test)\n' "$name"
        warnings=$((warnings + 1))
    fi
}

echo "SMY-OS environment check"
echo "Host: $(uname -srm)"
if [ -r /etc/os-release ]; then
    # shellcheck disable=SC1091
    . /etc/os-release
    echo "OS:   ${PRETTY_NAME:-unknown}"
fi
echo "CPUs: $(getconf _NPROCESSORS_ONLN 2>/dev/null || echo unknown)"
if [ -r /dev/kvm ] && [ -w /dev/kvm ]; then
    echo "KVM:  available (QEMU can use -accel kvm)"
else
    echo "KVM:  not available (QEMU runs with TCG; slower)"
fi
echo

echo "Build, unit-test and lint tools:"
need "clang" clang clang --version
need "ld.lld" ld.lld ld.lld --version
need "llvm-ar" llvm-ar llvm-ar --version
need "llvm-nm" llvm-nm llvm-nm --version
need "llvm-objdump" llvm-objdump llvm-objdump --version
need "llvm-objcopy" llvm-objcopy llvm-objcopy --version
need "llvm-readelf" llvm-readelf llvm-readelf --version
need "clang-format" clang-format clang-format --version
need "clang-tidy" clang-tidy clang-tidy --version
need "cppcheck" cppcheck cppcheck --version
need "make" make make --version
need "python3" python3 python3 --version
need "curl" curl curl --version
need "sha256sum" sha256sum sha256sum --version
need "tar" tar tar --version

if command -v clang >/dev/null 2>&1; then
    major=$(clang -dumpversion | cut -d. -f1)
    if [ "$major" -lt "$MIN_LLVM_MAJOR" ]; then
        printf '  FAIL  clang major version %s < %s (ADR 0003)\n' "$major" "$MIN_LLVM_MAJOR"
        errors=$((errors + 1))
    fi
    # Host unit tests link the sanitizer runtime (Ubuntu: libclang-rt-18-dev).
    asan_lib=$(clang -print-file-name=libclang_rt.asan-x86_64.a)
    if [ -f "$asan_lib" ]; then
        printf '  OK    %-20s %s\n' "sanitizer runtime" "$asan_lib"
    else
        printf '  MISS  %-20s (ASan runtime not found; install libclang-rt-%s-dev)\n' \
            "sanitizer runtime" "$major"
        errors=$((errors + 1))
    fi
fi
echo

echo "ISO and boot-test tools:"
later "qemu-system-x86_64" qemu-system-x86_64 qemu-system-x86_64 --version
later "xorriso" xorriso xorriso -version
later "mtools" mformat mformat --version
later "nasm (Limine build)" nasm nasm -v
later "rsvg-convert" rsvg-convert rsvg-convert --version
later "montage (ImageMagick)" montage montage -version
if [ -f /usr/share/OVMF/OVMF_CODE_4M.fd ]; then
    printf '  OK    %-20s %s\n' "OVMF" "/usr/share/OVMF/OVMF_CODE_4M.fd"
elif [ "$strict_all" -eq 1 ]; then
    printf '  MISS  %-20s (/usr/share/OVMF/OVMF_CODE_4M.fd)\n' "OVMF"
    errors=$((errors + 1))
else
    printf '  WARN  %-20s (not found; needed for make iso / boot-test)\n' "OVMF"
    warnings=$((warnings + 1))
fi
echo

if [ "$errors" -ne 0 ]; then
    echo "check-env: FAIL ($errors missing or too old, $warnings warnings)"
    exit 1
fi
echo "check-env: OK ($warnings warnings)"
