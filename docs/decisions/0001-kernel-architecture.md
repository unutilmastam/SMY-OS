# ADR 0001 — Kernel architecture: modular monolith

- **Status:** Accepted
- **Date:** 2026-10-17
- **Phase:** 0

## Context

SMY Kernel is written from scratch by a small team (Claude Code sessions driven
by one owner), tested almost entirely in QEMU through CI. It must later host
two compatibility personalities (Linux syscalls, NT-like services for PE
programs), a user-space display server, and drivers for real PCs (AHCI, NVMe,
xHCI). The master prompt (§3.1) asks for a modular monolith unless an ADR shows
something better.

Constraints that matter most:

- **Time to first useful result.** Each phase must end with something that boots
  and is tested in CI.
- **Debuggability with only serial logs.** No physical machine, no interactive
  debugger in most sessions.
- **Security.** User/kernel separation and driver isolation where practical.
- **Compatibility.** Linux and Windows personalities need fast, synchronous
  syscall paths and shared VM/file semantics.

## Decision

SMY Kernel is a **modular monolithic kernel**:

- One address space and one binary for core kernel, memory management,
  scheduler, VFS, drivers and syscall personalities.
- Strict **internal module boundaries**: each subsystem (`kernel/memory`,
  `kernel/scheduler`, `kernel/storage`, `kernel/drivers/…`) exposes a header
  with its public interface; other modules only include those headers. No
  reaching into another module's internals.
- Architecture-specific code only in `kernel/arch/x86_64/`; architecture-
  independent code never includes x86 headers.
- Portable logic (allocators, parsers, data structures) is written so it builds
  both in the kernel and as a host library for unit tests with sanitizers.
- **Everything that does not need ring 0 runs in user space:** display server,
  compositor, desktop, installer, package manager, live-environment services.
  The kernel provides memory mapping, input events and IPC.
- Drivers start in-kernel behind a driver-model interface (Phase 13). Moving
  selected drivers to user space is a later option, enabled by keeping the
  driver interface message-like (request/response) and capability-checked.

## Alternatives

| Option | Advantages | Disadvantages | Verdict |
|---|---|---|---|
| Pure monolith (no module rules) | Fastest to write | Becomes unmaintainable; hard to test pieces in isolation | Rejected |
| **Modular monolith** | Simple boot and debugging, fast syscalls, host-testable modules, proven model | A driver bug can crash the kernel | **Chosen** |
| Microkernel (drivers + FS as servers) | Strong isolation, restartable drivers | IPC design must be right from day one; much more code before anything boots; hard to debug from serial logs only; slower compat syscalls | Rejected for now |
| Hybrid (NT-style) | Flexible | Complexity of both without clear benefit at this stage | Rejected |
| Exokernel / unikernel | Research value | Does not fit a general-purpose desktop OS | Rejected |

## Consequences

- Early phases (2–16) stay small and testable.
- Driver robustness relies on code quality, sanitized host tests and fuzzing,
  plus kernel hardening (W^X, NX, SMEP/SMAP, stack protector).
- The IPC designed in Phase 11 must be good enough for the display server and
  later user-space drivers — it is a first-class kernel feature, not an add-on.
- Revisit when: driver crashes dominate bug reports, or a component handling
  untrusted input (e.g. USB, network) would be safer in user space.
