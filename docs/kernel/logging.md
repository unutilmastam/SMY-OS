# Kernel log

Status: **Implemented, Emulator-tested** (Phase 3). Code: `kernel/core/log.{h,c}`,
formatter `kernel/core/fmt.{h,c}`.

- Output: COM1 serial (16550, 115200 8N1, polled). Nothing is written to the
  screen yet (framebuffer text console: Phase 13).
- `klog(level, fmt, ...)` writes one line:
  - `LOG_INFO`: `SMY-OS: <message>`
  - `LOG_DEBUG`, `LOG_WARN`, `LOG_ERROR`: `SMY-OS: [debug|warn|error] <message>`
- Default level: info. `smy.loglevel=debug` on the command line enables
  debug messages.
- Format strings use the formatter's printf subset (`%d %i %u %x %X %p %c %s
  %%`, flags `-` `0`, width, `.N`/`.*` for strings, lengths `hh h l ll z`);
  the compiler checks them with `format(printf)` attributes. Unsupported
  conversions are printed literally and never read arguments or write memory.
- Stable lines that tests match: `SMY-OS: boot OK`, `SMY-OS: firmware: …`,
  `SMY-OS: PANIC: …`, `SMY-OS: ktest: …`. Change them only together with the
  tests.
- Not yet: timestamps (need a timer, Phase 7), a ring buffer readable from
  user space, locking for SMP (Phase 16).
