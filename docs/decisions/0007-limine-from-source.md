# ADR 0007 — Limine distribution: build from the verified release source tarball

- **Status:** Accepted (supersedes the "Binaries" row and the download detail of ADR 0005)
- **Date:** 2026-10-17
- **Phase:** 1

## Context

ADR 0005 assumed that Limine publishes prebuilt binary packages for every
point release and planned to download one. Checked online in Phase 1
(2026-10-17):

- `git ls-remote` on `github.com/limine-bootloader/limine`: the newest
  `v*-binary` tag is **`v9.6.7-binary`**. There are no binary tags or branches
  for 10.x, 11.x or 12.x.
- The `v12.9.3` GitHub release has the assets `limine-12.9.3.tar.xz`,
  `limine-12.9.3.tar.xz.sig` and `limine-12.9.3.tar.gz` (source only; no
  `SHA256SUMS`).
- `limine-12.9.3.tar.xz`: 580 844 bytes, SHA-256
  `95f456a204ce4855a9b5bd72e2cc170080f217edf5e7edf8ded9bb9f3dd4e995`
  (computed from the downloaded asset).

So the pinned version (12.9.3) has no official binaries.

## Decision

- `tools/fetch-limine.sh` downloads **`limine-12.9.3.tar.xz`**, verifies the
  SHA-256 above (recorded in the script; mismatch = hard failure), and unpacks
  it into `third_party/downloads/` (git-ignored).
- **Phase 2 builds Limine from this source** inside the build (`make iso`),
  with the project's pinned toolchain. Limine's own build needs `nasm` and
  `mtools` in addition to clang/lld; these are build tools only. ADR 0002's
  "NASM is not used" applies to SMY-OS sources and stays valid.
- The detached `.sig` (upstream GPG signature) is not verified: the SHA-256
  pin already fixes the exact bytes (trust-on-first-download, recorded here).
  Signature verification can be added if the upstream key's fingerprint is
  published in a place we can pin.
- Version stays **12.9.3**. ADR 0005's choice of Limine and the Limine
  protocol is unchanged.

## Alternatives

| Option | Why not chosen |
|---|---|
| Use `v9.6.7-binary` | Three major versions old; the protocol and fixes since then would be lost |
| Commit prebuilt Limine binaries to git | Unreviewable binary blobs in the repository; violates "generated files stay out of source control" |
| Build from a git clone of the tag | Release tarball is the official, checksummable artefact and contains the generated `configure` |

## Consequences

- One extra build step (a few seconds to a minute) in Phase 2 and CI.
- The bootloader binaries in our ISO are reproducible from a pinned source and
  a pinned toolchain.
- Upgrading Limine = one commit changing `LIMINE_VERSION` and `LIMINE_SHA256`
  in `tools/fetch-limine.sh` (and the ADR table).
