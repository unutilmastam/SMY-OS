# Limine boot protocol header (vendored)

- File: `include/limine.h`, unmodified.
- Source: `limine-protocol/include/limine.h` inside the Limine 12.9.3 release
  tarball `limine-12.9.3.tar.xz` (SHA-256
  `95f456a204ce4855a9b5bd72e2cc170080f217edf5e7edf8ded9bb9f3dd4e995`, fetched
  and verified by `tools/fetch-limine.sh`).
- SHA-256 of `include/limine.h`:
  `0aadf2633c85d8cb145344c968add158aeb1deb68af567d55ba8431c492b6d4c`.
- Licence: 0BSD, see [LICENSE](LICENSE).
- Why vendored: the kernel compiles against the exact protocol version the
  pinned bootloader implements (ADR 0005, ADR 0007).

Update only together with `LIMINE_VERSION` in `tools/fetch-limine.sh`, by
copying the file from the new verified tarball and updating the hash above.
