# Kernel command line

Status: **Implemented** (Phase 3). Parser: `kernel/core/cmdline.c`
(host-unit-tested). Source: the Limine executable command line request
(`cmdline:` in `limine.conf`). The normal ISO passes no options.

Format: space-separated tokens, `key` or `key=value`; values contain no
spaces; the last occurrence of a key wins. SMY-OS options use the `smy.`
prefix.

| Option | Values | Effect |
|---|---|---|
| `smy.ktest` | `all` or a test name | run in-kernel self-tests and exit QEMU (`docs/kernel/ktest.md`) |
| `smy.loglevel` | `debug` | log debug messages too (default: info) |

The kernel logs the command line at boot (`SMY-OS: command line: …`) when
it is not empty.
