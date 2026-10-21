/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

/*
 * ACPI static tables: RSDP -> RSDT/XSDT -> MADT, HPET (ACPI 6.5 §5.2;
 * docs/kernel/interrupts.md). Pure parsing, host-testable: physical memory
 * is reached only through `struct acpi_mem_ops`. Every table's checksum and
 * length is validated before it is used; nothing is written.
 */

#ifndef SMY_KERNEL_ACPI_ACPI_H
#define SMY_KERNEL_ACPI_ACPI_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define ACPI_MAX_TABLES 64
#define ACPI_MAX_IOAPICS 8
#define ACPI_MAX_OVERRIDES 24
#define ACPI_MAX_TABLE_SIZE 0x100000u /* 1 MiB: larger is treated as corrupt */

struct acpi_mem_ops {
    /* Readable pointer to [phys, phys+len); NULL if it cannot be mapped. */
    const void *(*map)(void *ctx, uint64_t phys, size_t len);
    void *ctx;
};

struct acpi_sdt_header {
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oem_id[6];
    char oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
} __attribute__((packed));

_Static_assert(sizeof(struct acpi_sdt_header) == 36, "ACPI SDT header is 36 bytes");

enum acpi_status {
    ACPI_OK = 0,
    ACPI_ERR_NO_RSDP,  /* null address or unmappable */
    ACPI_ERR_BAD_RSDP, /* signature or checksum */
    ACPI_ERR_BAD_ROOT, /* RSDT/XSDT signature, length or checksum */
    ACPI_ERR_NOT_FOUND,
    ACPI_ERR_BAD_TABLE, /* length or checksum of a table, malformed entries */
};

struct acpi {
    const struct acpi_mem_ops *ops;
    uint8_t revision;                 /* RSDP revision: 0 = ACPI 1.0 (RSDT), 2+ = XSDT */
    uint64_t tables[ACPI_MAX_TABLES]; /* physical addresses from the root table */
    size_t table_count;
    char oem_id[7];
};

struct acpi_ioapic {
    uint8_t id;
    uint32_t address;  /* physical MMIO base */
    uint32_t gsi_base; /* first global system interrupt it handles */
};

/* An ISA IRQ wired to a different GSI or with non-default polarity/trigger. */
struct acpi_override {
    uint8_t source_irq;
    uint32_t gsi;
    uint16_t flags; /* MPS INTI flags: bits 0-1 polarity, 2-3 trigger */
};

#define ACPI_INTI_POLARITY_MASK 0x3u
#define ACPI_INTI_POLARITY_LOW 0x3u
#define ACPI_INTI_TRIGGER_MASK 0xCu
#define ACPI_INTI_TRIGGER_LEVEL 0xCu

struct acpi_madt_info {
    uint64_t lapic_address; /* after any 64-bit override entry */
    bool has_legacy_pics;   /* PCAT_COMPAT: 8259 PICs present (to be masked) */
    unsigned cpu_count;     /* enabled or online-capable processor entries */
    struct acpi_ioapic ioapics[ACPI_MAX_IOAPICS];
    unsigned ioapic_count;
    struct acpi_override overrides[ACPI_MAX_OVERRIDES];
    unsigned override_count;
};

struct acpi_hpet_info {
    uint64_t address; /* physical MMIO base (system memory space) */
    uint8_t number;
};

enum acpi_status acpi_init(struct acpi *acpi, const struct acpi_mem_ops *ops, uint64_t rsdp_phys);

/* The `index`-th valid table with this signature (checksum verified). */
const struct acpi_sdt_header *acpi_find_table(const struct acpi *acpi, const char signature[4],
                                              unsigned index);

enum acpi_status acpi_parse_madt(const struct acpi_sdt_header *madt, struct acpi_madt_info *out);
enum acpi_status acpi_parse_hpet(const struct acpi_sdt_header *hpet, struct acpi_hpet_info *out);

/* GSI and flags for ISA IRQ `irq`: identity mapping unless overridden. */
uint32_t acpi_isa_irq_to_gsi(const struct acpi_madt_info *madt, uint8_t irq, uint16_t *flags_out);

/* True if `len` bytes at `p` sum to 0 modulo 256. */
bool acpi_checksum_ok(const void *p, size_t len);

const char *acpi_status_name(enum acpi_status status);

#endif /* SMY_KERNEL_ACPI_ACPI_H */
