#!/bin/sh
# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 The SMY-OS Authors
#
# Build the ISO twice from scratch (kernel, Limine, image) and require
# identical SHA-256 hashes (docs/install/iso-spec.md §5):
#   1. in this checkout, in build/repro-a;
#   2. in a copy of the source tree at a different absolute path, so that
#      paths leaking into the image (debug info, build-ids) are detected.
# Language: POSIX sh (ADR 0002).
#
# Usage: tools/check-reproducible.sh

set -eu

root=$(cd "$(dirname "$0")/.." && pwd)
cd "$root"

# Both builds must use the same timestamp; the copy has no .git directory.
SOURCE_DATE_EPOCH=${SOURCE_DATE_EPOCH:-$(git log -1 --format=%ct)}
export SOURCE_DATE_EPOCH

a="$root/build/repro-a"
tmp=$(mktemp -d)
copy="$tmp/elsewhere/SMY-OS"
trap 'rm -rf "$tmp"' EXIT INT TERM
rm -rf "$a"

echo "check-reproducible: build 1/2 in $a"
make --no-print-directory BUILD="$a" iso >/dev/null

# Copy tracked and untracked-but-not-ignored files (the working tree as it
# would be committed) plus the verified Limine tarball cache.
mkdir -p "$copy/third_party/downloads"
git ls-files -z --cached --others --exclude-standard | tar -C "$root" --null -T - -cf - |
    tar -C "$copy" -xf -
cp third_party/downloads/limine-*.tar.xz "$copy/third_party/downloads/"

echo "check-reproducible: build 2/2 in $copy/build"
make --no-print-directory -C "$copy" iso >/dev/null

hash_a=$(sha256sum "$a/smy-os.iso" | cut -d' ' -f1)
hash_b=$(sha256sum "$copy/build/smy-os.iso" | cut -d' ' -f1)
echo "  $hash_a  build 1 (this checkout)"
echo "  $hash_b  build 2 (copy at another path)"

if [ "$hash_a" != "$hash_b" ]; then
    echo "check-reproducible: FAIL — the two ISOs differ" >&2
    cmp -l "$a/smy-os.iso" "$copy/build/smy-os.iso" | head -n 20 >&2 || true
    exit 1
fi
rm -rf "$a"
echo "check-reproducible: OK (identical ISOs)"
