# ADR 0012 — Virtual memory: own page tables, guarded stacks, slab heap

- **Status:** Accepted
- **Date:** 2026-10-21
- **Phase:** 5

## Context

Until Phase 5 the kernel ran on Limine's page tables: every kernel segment
had the rights of its ELF program header, but the kernel could not map,
unmap or protect anything, kernel stacks had no guard pages, and there was
no heap. Phase 5 requires own 4-level page tables, a direct map, NX and W^X
for kernel sections, SMEP/SMAP when supported, page-fault diagnostics, a
kernel heap and guard pages for kernel stacks — all testable on the host
where possible and in QEMU otherwise.

## Decision

1. **Own page tables, same direct-map offset.** `vmm_init` builds a new
   PML4 with (a) a direct map of every memory-map entry except *reserved* and
   *bad memory*, at the offset Limine reported, so pointers handed over by
   the bootloader (responses, framebuffer, command line) stay valid; and
   (b) the kernel image mapped section by section from linker symbols:
   requests RW, text RX, rodata R, data/bss RW. Everything not executable
   carries NX. Then it loads CR3.
2. **Page-table code in two layers.** `arch/x86_64/paging.c` is pure table
   logic behind `struct paging_ops` (allocate/reach a table page, invalidate
   a TLB entry), host-tested on a fake page pool. `arch/mmu.h` is the
   architecture-neutral interface generic code uses (`arch_mmu_map`, …),
   following the portability rule (`docs/architecture/hardware.md`).
3. **Large pages for the direct map.** 2 MiB pages where both addresses are
   aligned, 4 KiB pages at the edges. 1 GiB pages are not used (not
   universally supported; little gain at current memory sizes).
4. **CPU protections.** NX is required (panic without it: every x86-64 CPU
   SMY-OS targets has it). CR0.WP on, so kernel writes honour read-only
   pages. SMEP and SMAP switched on when CPUID reports them. PAT entry 1
   becomes write-combining (Linux uses the same layout) so the framebuffer is
   mapped WC; without PAT it stays write-back.
5. **Guarded kernel stacks.** Stacks live in their own region
   (`0xffffff0000000000`), each in a 1 MiB slot with an unmapped guard page
   below it. After `vmm_init` the kernel switches to a 64 KiB guarded main
   stack and moves the IST stacks (#DF, NMI, #MC) to guarded 16 KiB stacks.
   A stack overflow therefore hits the guard page; the #PF cannot be
   delivered on the same stack, the CPU raises #DF on its IST stack, and the
   panic report names the overflowed stack.
6. **Heap: size classes + page runs.** `memory/heap.c` (host-tested):
   seven classes 16…1024 bytes in one-page slabs with a 64-byte header and a
   free bitmap; larger blocks are page runs with the same header. Every
   block is 16-byte aligned; `kfree` finds the header at `ptr & ~4095`.
   Invalid pointers and double frees are detected (panic in `kfree`);
   freed slots are poisoned with `0xDF`. Pages come from the PMM through the
   direct map (`memory/kmalloc.c`).

## Alternatives considered

- **Keep Limine's tables and only add mappings:** no control over rights or
  caching of existing mappings and nothing to test against; rejected.
- **Recursive page-table mapping:** unnecessary with a direct map of all
  RAM, and harder to reason about; rejected.
- **A virtual heap region with on-demand mapping:** needed later for very
  large or long-lived allocations without contiguous RAM; not needed now.
  Large blocks currently require physically contiguous pages.
- **Buddy allocator for the heap:** more fragmentation control for large
  blocks, more code; the PMM already handles page runs.

## Consequences

- The kernel controls every mapping; W^X is checked by a ktest over the
  whole image, and writes to read-only pages, execution from NX pages and
  stack overflows are tested in QEMU.
- Page-table pages are never freed (the kernel address space only grows);
  user address spaces (Phase 10) will free whole trees.
- Bootloader-reclaimable memory (Limine's old tables, responses) is still
  not reclaimed; that happens once nothing reads the responses.
- Not thread-safe; locking arrives with SMP (Phase 16).
- No global (G) bit on kernel pages yet: a TLB optimisation for when
  address-space switches exist (Phase 10/11).
