# Kernel memory

Status: **physical memory manager (Phase 4), virtual memory and kernel heap
(Phase 5) — Implemented, Unit-tested, Emulator-tested.**

## Physical memory manager

Code: `kernel/memory/pmm.{h,c}` (portable core), `kernel/memory/phys.{h,c}`
(Limine glue and global API). Design: [ADR 0010](../decisions/0010-physical-memory-bitmap.md).

### Boot sequence

1. `kmain` requests the Limine **memory map** and **HHDM** (higher-half direct
   map, virtual = physical + offset).
2. `phys_init()` copies the map (max 512 entries), logs each entry at debug
   level (`smy.loglevel=debug`), sizes the bitmaps for the highest usable
   address, places them in the first usable region that fits, and builds the
   allocator.
3. It logs one line, e.g. (QEMU q35, 256 MiB, SeaBIOS):

```
SMY-OS: memory: 254 MiB usable in 20 map entries, 65079 free pages, bitmap 16 KiB
```

Measured (Phase 4, QEMU 8.2): SeaBIOS 256 MiB → 254 MiB usable, 65 079 free
pages; OVMF 256 MiB → 207–208 MiB usable, about 53 100 free pages (OVMF keeps
more memory for itself and the exact amount varies slightly between boots); OVMF 4 GiB → 4 047 MiB usable, 1 036 060 free pages,
bitmap 384 KiB.

### What is managed

| Limine type | Managed now | Notes |
|---|---|---|
| usable | yes | whole 4 KiB pages only |
| bootloader reclaimable | no | holds the Limine responses (and Limine's old page tables); reclaimed once nothing reads the responses |
| ACPI reclaimable | no | ACPI tables, needed in Phase 7 |
| kernel and modules, framebuffer, reserved, ACPI NVS, bad memory | never | |

Frame 0 and the bitmaps' own frames are never handed out.

### API (`memory/phys.h`)

```c
enum pmm_status phys_alloc_page(uint64_t *phys_out);
enum pmm_status phys_alloc_pages(uint64_t count, uint64_t align_pages, uint64_t *phys_out);
enum pmm_status phys_free_page(uint64_t phys);
enum pmm_status phys_free_pages(uint64_t phys, uint64_t count);
struct pmm_stats phys_stats(void);
void *phys_to_virt(uint64_t phys);
```

Errors: `PMM_ERR_NO_MEMORY`, `PMM_ERR_INVALID_ADDRESS` (misaligned, outside
the map, or a frame the PMM does not own), `PMM_ERR_DOUBLE_FREE`,
`PMM_ERR_BAD_ARGUMENT`. Freeing a contiguous range checks every frame first
and changes nothing if one is invalid. Pages are **not** zeroed.

### Tests

- Host (`tests/unit/kernel/memory/test_pmm.c`, ASan/UBSan): a PC-like map
  with holes, a reserved area inside RAM, an unaligned region and MMIO above
  RAM; exact free-frame count; every frame handed out once; exhaustion;
  address validation and double free; contiguous allocation with alignment
  around a hole; overlapping/degenerate maps; 20 000 random alloc/free steps
  against a reference model.
- Kernel (`kernel/memory/ktest_phys.c`, `make ktest`):
  `phys_allocates_all_memory_and_frees_without_leak` takes every free page,
  writes a linked-list node into each (proving the page is writable RAM and
  handed out once), checks each lies in a usable map entry, frees all and
  checks the free count is restored; contiguous allocation; double-free and
  foreign-frame rejection. Runs at 256 MiB on BIOS and UEFI and at 4 GiB on
  UEFI (`make ktest-big-memory`, memory above 4 GiB).

### Limits

- Not thread-safe (single CPU until Phase 16).
- Contiguous allocation is a linear scan.
- No NUMA, no memory hot-plug, no per-zone (DMA below 16 MiB/4 GiB) policy
  yet — added when a driver needs it.

## Virtual memory

Code: `kernel/arch/x86_64/paging.{h,c}` (4-level table logic, host-tested),
`kernel/arch/x86_64/mmu.c` (CPU side: CR3, EFER.NXE, CR0.WP, PAT, SMEP,
SMAP), `kernel/arch/mmu.h` (architecture-neutral interface),
`kernel/memory/vmm.{h,c}` (kernel address space, guarded stacks). Design:
[ADR 0012](../decisions/0012-virtual-memory-and-heap.md).

### Kernel address space

| Range | Contents | Rights |
|---|---|---|
| `0xffff800000000000` + phys | direct map of every memory-map entry except reserved and bad memory (2 MiB pages where aligned) | RW, NX; framebuffer write-combining |
| `0xfffffe0000000000` … +256 GiB | scratch area for ktests | as mapped by the test |
| `0xffffff0000000000` … +256 GiB | kernel stacks: 1 MiB slots, unmapped guard page + stack | RW, NX |
| `0xffffffff80000000` | kernel image: requests / text / rodata / data+bss | RW NX / RX / R NX / RW NX |

Everything else is unmapped. The direct map uses the offset Limine reported,
so bootloader pointers stay valid after the switch.

### Boot sequence

1. `phys_init` (above), then `vmm_init`: new PML4 from the PMM, direct map,
   kernel sections from linker symbols (`kernel_start`, `kernel_text_*`,
   `kernel_rodata_*`, `kernel_data_start`, `kernel_end`), then
   `arch_mmu_activate`: EFER.NXE (panic without NX), PAT entry 1 = WC,
   CR3 load, full TLB flush, CR0.WP, SMEP/SMAP if CPUID has them.
2. Switch to a 64 KiB guarded main stack (`arch_switch_stack`), move the
   IST stacks (#DF, NMI, #MC) to guarded 16 KiB stacks, start the heap.

Log line (QEMU q35, 256 MiB, SeaBIOS, default CPU):

```
SMY-OS: vmm: own page tables active; direct map 259 MiB, tables 40 KiB; NX on, WP on, SMEP off, SMAP off, WC on
```

With `-cpu max` SMEP and SMAP are on. OVMF needs about 76 KiB of tables
(more, smaller map entries); 4 GiB of RAM about 88 KiB.

### API

```c
/* arch/mmu.h — kernel address space */
enum vm_status arch_mmu_map(uint64_t virt, uint64_t phys, uint64_t size, unsigned flags);
enum vm_status arch_mmu_unmap(uint64_t virt, uint64_t *phys_out);
enum vm_status arch_mmu_protect(uint64_t virt, unsigned flags);
enum vm_status arch_mmu_translate(uint64_t virt, struct vm_translation *out);
/* flags: VM_READ (0) | VM_WRITE | VM_EXEC | VM_USER | VM_WRITE_COMBINING | VM_UNCACHED */

/* memory/vmm.h */
uintptr_t vmm_alloc_stack(size_t pages);  /* guard page below, returns the top */
bool vmm_stack_bounds(uintptr_t addr, uintptr_t *lo, uintptr_t *hi);
bool vmm_stack_guard_hit(uintptr_t addr, uintptr_t *lo, uintptr_t *hi);
```

Errors: `VM_ERR_NO_MEMORY`, `VM_ERR_BAD_ARGUMENT` (misaligned,
non-canonical, unknown flags), `VM_ERR_ALREADY_MAPPED` (mapping never
silently replaces another), `VM_ERR_NOT_MAPPED`, `VM_ERR_LARGE_PAGE` (a
4 KiB operation inside a 2 MiB page). Page-table pages are never freed.

## Kernel heap

Code: `kernel/memory/heap.{h,c}` (host-tested core),
`kernel/memory/kmalloc.{h,c}` (kernel glue).

```c
void *kmalloc(size_t size);   /* 16-byte aligned, not zeroed; NULL on failure or size 0 */
void *kzalloc(size_t size);
void kfree(void *ptr);        /* NULL ignored; invalid pointer or double free -> panic */
```

- Small blocks (1–1024 bytes): classes 16, 32, 64, 128, 256, 512, 1024 in
  one-page slabs (64-byte header with magic, class, free bitmap). One empty
  slab per class is kept; other empty slabs go back to the PMM.
- Large blocks (> 1024 bytes): a run of physically contiguous pages with the
  same header in front; returned to the PMM on `kfree`.
- Freed small slots are filled with `0xDF`; released pages are marked so a
  second free is still reported. Limit: a stale pointer into a page that was
  released and reused can pass the checks.

## Tests (Phase 5)

- Host: `tests/unit/kernel/arch/test_paging.c` (9 tests: translation and
  hardware bits, user/attribute bits, bad arguments, double map, unmap and
  TLB invalidation, protect, 2 MiB pages, mixed ranges, out-of-memory
  without corruption); `tests/unit/kernel/memory/test_heap.c` (7 tests incl.
  30 000 random steps with content checks, poisoning, invalid pointers and
  double frees, slab release).
- Kernel (`make ktest`): CPU protections match CPUID; exact rights of text,
  rodata, data and direct map; no page of the image is writable and
  executable; map → write → read through the direct map → unmap → fault;
  write to a read-only page and to `.rodata` faults; executing from an NX
  page faults (instruction fetch); stacks and IST stacks have guard pages;
  heap stress (20 000 random steps, no leak), 1 MiB block, `kzalloc`.
  `make ktest-cpu-max` runs the suite with SMEP/SMAP on.
- Panics: `make ktest-panic-stack-overflow` (#DF on the IST stack with the
  stack-overflow line), `make ktest-panic-double-kfree`.

