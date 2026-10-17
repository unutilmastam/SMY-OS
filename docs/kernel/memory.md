# Kernel memory

Status (Phase 4): **physical memory manager — Implemented, Unit-tested,
Emulator-tested.** Virtual memory and the kernel heap follow in Phase 5.

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
| bootloader reclaimable | no | holds the Limine responses still in use; reclaim after Phase 5 |
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
