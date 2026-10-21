/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

#include "acpi/acpi.h"

/* RSDP (ACPI 6.5 §5.2.5.3): 20 bytes in revision 0, 36 bytes from revision 2. */
struct rsdp {
    char signature[8];
    uint8_t checksum;
    char oem_id[6];
    uint8_t revision;
    uint32_t rsdt_address;
    uint32_t length;
    uint64_t xsdt_address;
    uint8_t extended_checksum;
    uint8_t reserved[3];
} __attribute__((packed));

_Static_assert(sizeof(struct rsdp) == 36, "RSDP is 36 bytes");

/* MADT entry types (ACPI 6.5 §5.2.12). */
enum {
    MADT_LOCAL_APIC = 0,
    MADT_IO_APIC = 1,
    MADT_OVERRIDE = 2,
    MADT_LAPIC_ADDRESS_OVERRIDE = 5,
    MADT_LOCAL_X2APIC = 9,
};

#define MADT_HEADER_SIZE 44u /* SDT header + local APIC address + flags */
#define HPET_TABLE_SIZE 56u

bool acpi_checksum_ok(const void *p, size_t len)
{
    const uint8_t *b = p;
    uint8_t sum = 0;

    for (size_t i = 0; i < len; i++) {
        sum = (uint8_t)(sum + b[i]);
    }
    return sum == 0;
}

static bool same_signature(const char *a, const char *b, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

static uint32_t read_u32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static uint64_t read_u64(const uint8_t *p)
{
    return (uint64_t)read_u32(p) | (uint64_t)read_u32(p + 4) << 32;
}

/* Map a whole table: header first for the length, then all of it. */
static const struct acpi_sdt_header *map_table(const struct acpi_mem_ops *ops, uint64_t phys)
{
    const struct acpi_sdt_header *h = ops->map(ops->ctx, phys, sizeof *h);

    if (h == NULL || h->length < sizeof *h || h->length > ACPI_MAX_TABLE_SIZE) {
        return NULL;
    }
    h = ops->map(ops->ctx, phys, h->length);
    if (h == NULL || !acpi_checksum_ok(h, h->length)) {
        return NULL;
    }
    return h;
}

enum acpi_status acpi_init(struct acpi *acpi, const struct acpi_mem_ops *ops, uint64_t rsdp_phys)
{
    const struct rsdp *r;
    const struct acpi_sdt_header *root;
    uint64_t root_phys;
    size_t entry_size;

    acpi->ops = ops;
    acpi->table_count = 0;
    if (rsdp_phys == 0) {
        return ACPI_ERR_NO_RSDP;
    }
    r = ops->map(ops->ctx, rsdp_phys, 20);
    if (r == NULL) {
        return ACPI_ERR_NO_RSDP;
    }
    if (!same_signature(r->signature, "RSD PTR ", 8) || !acpi_checksum_ok(r, 20)) {
        return ACPI_ERR_BAD_RSDP;
    }
    acpi->revision = r->revision;
    for (int i = 0; i < 6; i++) {
        acpi->oem_id[i] = r->oem_id[i];
    }
    acpi->oem_id[6] = '\0';

    if (r->revision >= 2) {
        r = ops->map(ops->ctx, rsdp_phys, sizeof *r);
        if (r == NULL || r->length < sizeof *r || !acpi_checksum_ok(r, sizeof *r)) {
            return ACPI_ERR_BAD_RSDP;
        }
    }
    if (r->revision >= 2 && r->xsdt_address != 0) {
        root_phys = r->xsdt_address;
        entry_size = 8;
    } else {
        root_phys = r->rsdt_address;
        entry_size = 4;
    }
    root = map_table(ops, root_phys);
    if (root == NULL || !same_signature(root->signature, entry_size == 8 ? "XSDT" : "RSDT", 4)) {
        return ACPI_ERR_BAD_ROOT;
    }
    {
        const uint8_t *entries = (const uint8_t *)root + sizeof *root;
        const size_t n = (root->length - sizeof *root) / entry_size;

        for (size_t i = 0; i < n && acpi->table_count < ACPI_MAX_TABLES; i++) {
            const uint64_t phys =
                entry_size == 8 ? read_u64(entries + i * 8) : read_u32(entries + i * 4);

            if (phys != 0) {
                acpi->tables[acpi->table_count++] = phys;
            }
        }
    }
    return ACPI_OK;
}

const struct acpi_sdt_header *acpi_find_table(const struct acpi *acpi, const char signature[4],
                                              unsigned index)
{
    for (size_t i = 0; i < acpi->table_count; i++) {
        const struct acpi_sdt_header *h = map_table(acpi->ops, acpi->tables[i]);

        if (h != NULL && same_signature(h->signature, signature, 4)) {
            if (index == 0) {
                return h;
            }
            index--;
        }
    }
    return NULL;
}

enum acpi_status acpi_parse_madt(const struct acpi_sdt_header *madt, struct acpi_madt_info *out)
{
    const uint8_t *base = (const uint8_t *)madt;
    size_t off = MADT_HEADER_SIZE;

    *out = (struct acpi_madt_info){0};
    if (madt == NULL || !same_signature(madt->signature, "APIC", 4) ||
        madt->length < MADT_HEADER_SIZE) {
        return ACPI_ERR_BAD_TABLE;
    }
    out->lapic_address = read_u32(base + 36);
    out->has_legacy_pics = (read_u32(base + 40) & 1u) != 0;

    while (off + 2 <= madt->length) {
        const uint8_t type = base[off];
        const uint8_t len = base[off + 1];
        const uint8_t *e = base + off;

        if (len < 2 || off + len > madt->length) {
            return ACPI_ERR_BAD_TABLE;
        }
        switch (type) {
        case MADT_LOCAL_APIC:
            /* flags bit 0 enabled, bit 1 online capable */
            if (len >= 8 && (read_u32(e + 4) & 3u) != 0) {
                out->cpu_count++;
            }
            break;
        case MADT_LOCAL_X2APIC:
            if (len >= 16 && (read_u32(e + 8) & 3u) != 0) {
                out->cpu_count++;
            }
            break;
        case MADT_IO_APIC:
            if (len < 12) {
                return ACPI_ERR_BAD_TABLE;
            }
            if (out->ioapic_count < ACPI_MAX_IOAPICS) {
                out->ioapics[out->ioapic_count++] = (struct acpi_ioapic){
                    .id = e[2], .address = read_u32(e + 4), .gsi_base = read_u32(e + 8)};
            }
            break;
        case MADT_OVERRIDE:
            if (len < 10) {
                return ACPI_ERR_BAD_TABLE;
            }
            /* bus 0 = ISA is the only defined value */
            if (e[2] == 0 && out->override_count < ACPI_MAX_OVERRIDES) {
                out->overrides[out->override_count++] =
                    (struct acpi_override){.source_irq = e[3],
                                           .gsi = read_u32(e + 4),
                                           .flags = (uint16_t)(e[8] | e[9] << 8)};
            }
            break;
        case MADT_LAPIC_ADDRESS_OVERRIDE:
            if (len < 12) {
                return ACPI_ERR_BAD_TABLE;
            }
            out->lapic_address = read_u64(e + 4);
            break;
        default:
            break; /* NMI sources, GIC, etc.: not used on x86 yet */
        }
        off += len;
    }
    return out->ioapic_count > 0 ? ACPI_OK : ACPI_ERR_BAD_TABLE;
}

enum acpi_status acpi_parse_hpet(const struct acpi_sdt_header *hpet, struct acpi_hpet_info *out)
{
    const uint8_t *b = (const uint8_t *)hpet;

    if (hpet == NULL || !same_signature(hpet->signature, "HPET", 4) ||
        hpet->length < HPET_TABLE_SIZE) {
        return ACPI_ERR_BAD_TABLE;
    }
    /* Generic address structure at offset 40: space id 0 = system memory. */
    if (b[40] != 0) {
        return ACPI_ERR_BAD_TABLE;
    }
    out->address = read_u64(b + 44);
    out->number = b[52];
    return out->address != 0 ? ACPI_OK : ACPI_ERR_BAD_TABLE;
}

uint32_t acpi_isa_irq_to_gsi(const struct acpi_madt_info *madt, uint8_t irq, uint16_t *flags_out)
{
    for (unsigned i = 0; i < madt->override_count; i++) {
        if (madt->overrides[i].source_irq == irq) {
            *flags_out = madt->overrides[i].flags;
            return madt->overrides[i].gsi;
        }
    }
    *flags_out = 0; /* conforms to the bus: ISA = active high, edge */
    return irq;
}

const char *acpi_status_name(enum acpi_status status)
{
    switch (status) {
    case ACPI_OK:
        return "ok";
    case ACPI_ERR_NO_RSDP:
        return "no RSDP";
    case ACPI_ERR_BAD_RSDP:
        return "invalid RSDP";
    case ACPI_ERR_BAD_ROOT:
        return "invalid RSDT/XSDT";
    case ACPI_ERR_NOT_FOUND:
        return "table not found";
    case ACPI_ERR_BAD_TABLE:
        return "invalid table";
    }
    return "unknown";
}
