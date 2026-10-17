#!/bin/sh
# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 The SMY-OS Authors
#
# Build the ISO twice from scratch (kernel, Limine, image) in two separate
# build directories and require identical SHA-256 hashes
# (docs/install/iso-spec.md §5). Language: POSIX sh (ADR 0002).
#
# Usage: tools/check-reproducible.sh

set -eu

root=$(cd "$(dirname "$0")/.." && pwd)
cd "$root"

a=build/repro-a
b=build/repro-b
rm -rf "$a" "$b"

echo "check-reproducible: build 1/2 in $a"
make --no-print-directory BUILD="$a" iso >/dev/null
echo "check-reproducible: build 2/2 in $b"
make --no-print-directory BUILD="$b" iso >/dev/null

hash_a=$(sha256sum "$a/smy-os.iso" | cut -d' ' -f1)
hash_b=$(sha256sum "$b/smy-os.iso" | cut -d' ' -f1)
echo "  $hash_a  $a/smy-os.iso"
echo "  $hash_b  $b/smy-os.iso"

if [ "$hash_a" != "$hash_b" ]; then
    echo "check-reproducible: FAIL — the two ISOs differ" >&2
    cmp -l "$a/smy-os.iso" "$b/smy-os.iso" | head -n 20 >&2 || true
    exit 1
fi
rm -rf "$a" "$b"
echo "check-reproducible: OK (identical ISOs)"
