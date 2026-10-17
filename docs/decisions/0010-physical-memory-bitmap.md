# ADR 0010 — Physical memory manager: bitmap allocator

- **Status:** Accepted
- **Date:** 2026-10-18
- **Phase:** 4

## Context

The kernel needs a page-frame allocator before it can build its own page
tables (Phase 5), a heap, stacks and process memory. Requirements now:
single 4 KiB frames, occasional physically contiguous runs (page tables,
later DMA buffers), detection of double frees and frees of memory the
allocator never owned, testability on the host, and no dynamic memory to
bootstrap itself. The memory map comes from Limine (tens of entries, holes
below 1 MiB and around the PCI window, RAM above 4 GiB).

## Decision

A **two-bitmap allocator** (`kernel/memory/pmm.c`):

- `managed` (usable RAM owned by the PMM) and `used` (allocated) bits per
  4 KiB frame, covering frame 0 up to the end of the highest usable region.
  Memory cost: 2 bits per 4 KiB = 64 KiB per 1 GiB of address space covered
  (16 KiB for 256 MiB, 384 KiB for a 4 GiB QEMU machine — measured).
- Storage for the bitmaps is taken from the first usable region that fits and
  reserved in the bitmaps themselves.
- Single frames: next-fit scan over 64-bit words (`free = managed & ~used`,
  count-trailing-zeros), O(1) amortised while memory is plentiful.
- Contiguous runs: linear scan with alignment, O(frames) — acceptable while
  only page tables and rare buffers need it.
- Only `USABLE` map entries are managed; reserved entries win over usable
  ones where a (buggy) map overlaps them; partial pages are never used;
  frame 0 is never handed out.
- Errors are returned as `enum pmm_status`; callers decide whether a failure
  is fatal.

## Alternatives

| Option | Advantages | Disadvantages | Verdict |
|---|---|---|---|
| **Two bitmaps** | Simple, compact, deterministic; detects double free and foreign frees; easy to test against a model | Contiguous allocations are O(n); single-frame scans degrade when memory is almost full | **Chosen** for Phases 4–15 |
| Buddy allocator | O(log n) for power-of-two blocks, cheap coalescing | More code and state; needs per-order free lists inside free pages (harder to validate, frees cannot be checked as cheaply) | Revisit with SMP and real workloads (Phase 16+) with benchmarks |
| Free-page stack/list | O(1) alloc/free | Cannot allocate contiguous runs; detecting double free needs extra state; free list lives in free pages (corruption risk) | Rejected |

## Consequences

- One `struct pmm` holds all state; the kernel instance is wrapped by
  `memory/phys.c`. Not thread-safe: a lock is added with SMP (Phase 16).
- Bootloader-reclaimable memory (Limine responses) and ACPI-reclaimable
  memory are not yet returned to the allocator (about 45 MiB under OVMF at
  256 MiB); reclaiming them is scheduled after Phase 5 and Phase 7.
- If allocation speed matters later, the bitmap can gain a per-word summary
  level or be replaced by a buddy allocator behind the same `phys_*` API.
