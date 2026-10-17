#!/bin/sh
# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 The SMY-OS Authors
#
# Download the pinned Limine release source tarball, verify its SHA-256 and
# unpack it into third_party/downloads/ (git-ignored). Building Limine from
# this source happens in Phase 2 (ADR 0007).
# Language: POSIX sh (ADR 0002).
#
# Upgrading Limine is an explicit commit that changes LIMINE_VERSION and
# LIMINE_SHA256 together (ADR 0005, ADR 0007).

set -eu

LIMINE_VERSION=12.9.3
LIMINE_SHA256=95f456a204ce4855a9b5bd72e2cc170080f217edf5e7edf8ded9bb9f3dd4e995
LIMINE_URL="https://github.com/limine-bootloader/limine/releases/download/v${LIMINE_VERSION}/limine-${LIMINE_VERSION}.tar.xz"

root=$(cd "$(dirname "$0")/.." && pwd)
dest="$root/third_party/downloads"
tarball="$dest/limine-${LIMINE_VERSION}.tar.xz"
srcdir="$dest/limine-${LIMINE_VERSION}"

verify() {
    echo "${LIMINE_SHA256}  $1" | sha256sum -c --status
}

mkdir -p "$dest"

if [ -f "$tarball" ] && verify "$tarball"; then
    echo "fetch-limine: cached $tarball (SHA-256 OK)"
else
    rm -f "$tarball" "$tarball.part"
    echo "fetch-limine: downloading Limine ${LIMINE_VERSION}"
    curl --fail --silent --show-error --location --retry 3 \
        --output "$tarball.part" "$LIMINE_URL"
    if ! verify "$tarball.part"; then
        echo "fetch-limine: SHA-256 MISMATCH for $LIMINE_URL" >&2
        echo "  expected: ${LIMINE_SHA256}" >&2
        echo "  got:      $(sha256sum "$tarball.part" | cut -d' ' -f1)" >&2
        rm -f "$tarball.part"
        exit 1
    fi
    mv "$tarball.part" "$tarball"
    echo "fetch-limine: SHA-256 OK"
fi

# Unpack fresh every time, so a modified tree can never be used by accident.
rm -rf "$srcdir"
tar -xJf "$tarball" -C "$dest"
if [ ! -f "$srcdir/COPYING" ]; then
    echo "fetch-limine: unexpected tarball layout (no $srcdir/COPYING)" >&2
    exit 1
fi
echo "fetch-limine: Limine ${LIMINE_VERSION} source ready in ${srcdir#"$root"/}"
