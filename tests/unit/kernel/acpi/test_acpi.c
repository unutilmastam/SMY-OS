/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

/* Host tests for kernel/acpi/acpi.c on fake firmware tables. */

#include "acpi/acpi.h"
#include "harness/smytest.h"

#include <string.h>

#define MEM_SIZE 0x10000u
#define RSDP_AT 0x1000u
#define RSDT_AT 0x2000u
#define XSDT_AT 0x3000u
#define MADT_AT 0x4000u
#define HPET_AT 0x5000u
#define FACP_AT 0x6000u

static uint8_t mem[MEM_SIZE];

/* 6 bytes, space-padded, no terminator (ACPI OEM ID). */
static const uint8_t oem_id[6] = {'S', 'M', 'Y', 'O', 'S', ' '};

static const void *fake_map(void *ctx, uint64_t phys, size_t len)
{
    (void)ctx;
    return phys + len <= MEM_SIZE ? mem + phys : NULL;
}

static const struct acpi_mem_ops ops = {.map = fake_map, .ctx = NULL};

static void put32(uint8_t *p, uint32_t v)
{
    for (int i = 0; i < 4; i++) {
        p[i] = (uint8_t)(v >> (8 * i));
    }
}

static void put64(uint8_t *p, uint64_t v)
{
    put32(p, (uint32_t)v);
    put32(p + 4, (uint32_t)(v >> 32));
}

static void fix_checksum(uint8_t *p, size_t len, size_t checksum_offset)
{
    uint8_t sum = 0;

    p[checksum_offset] = 0;
    for (size_t i = 0; i < len; i++) {
        sum = (uint8_t)(sum + p[i]);
    }
    p[checksum_offset] = (uint8_t)(0x100 - sum);
}

/* Header + body; returns the table length. */
static uint32_t make_table(uint32_t at, const char sig[4], const uint8_t *body, uint32_t body_len)
{
    uint8_t *t = mem + at;
    const uint32_t len = 36 + body_len;

    memset(t, 0, len);
    memcpy(t, sig, 4);
    put32(t + 4, len);
    t[8] = 1;
    memcpy(t + 10, oem_id, sizeof oem_id);
    memcpy(t + 36, body, body_len);
    fix_checksum(t, len, 9);
    return len;
}

static void make_rsdp(unsigned revision, uint32_t rsdt, uint64_t xsdt)
{
    uint8_t *r = mem + RSDP_AT;

    memset(r, 0, 36);
    memcpy(r, "RSD PTR ", 8);
    memcpy(r + 9, oem_id, sizeof oem_id);
    r[15] = (uint8_t)revision;
    put32(r + 16, rsdt);
    put32(r + 20, 36);
    put64(r + 24, xsdt);
    fix_checksum(r, 20, 8);
    fix_checksum(r, 36, 32);
}

/* A MADT like QEMU q35: 2 CPUs (one disabled entry), one I/O APIC,
 * IRQ0 -> GSI 2, IRQ9 level/low. */
static uint32_t make_madt(bool with_lapic_override)
{
    uint8_t body[128];
    uint32_t n = 0;

    memset(body, 0, sizeof body);
    put32(body, 0xFEE00000u); /* local APIC address */
    put32(body + 4, 1);       /* PCAT_COMPAT */
    n = 8;
    /* local APIC, enabled */
    body[n] = 0, body[n + 1] = 8, body[n + 2] = 0, body[n + 3] = 0, put32(body + n + 4, 1), n += 8;
    /* local APIC, enabled */
    body[n] = 0, body[n + 1] = 8, body[n + 2] = 1, body[n + 3] = 1, put32(body + n + 4, 1), n += 8;
    /* local APIC, neither enabled nor online capable */
    body[n] = 0, body[n + 1] = 8, body[n + 2] = 2, body[n + 3] = 2, put32(body + n + 4, 0), n += 8;
    /* I/O APIC id 0 at 0xFEC00000, GSI base 0 */
    body[n] = 1, body[n + 1] = 12, body[n + 2] = 0;
    put32(body + n + 4, 0xFEC00000u), put32(body + n + 8, 0), n += 12;
    /* override: ISA IRQ 0 -> GSI 2, flags 0 */
    body[n] = 2, body[n + 1] = 10, body[n + 2] = 0, body[n + 3] = 0, put32(body + n + 4, 2);
    n += 10;
    /* override: ISA IRQ 9 -> GSI 9, active low, level */
    body[n] = 2, body[n + 1] = 10, body[n + 2] = 0, body[n + 3] = 9, put32(body + n + 4, 9);
    body[n + 8] = 0x0F, n += 10;
    /* local APIC NMI (type 4): ignored */
    body[n] = 4, body[n + 1] = 6, n += 6;
    if (with_lapic_override) {
        body[n] = 5, body[n + 1] = 12, put64(body + n + 4, 0x00000000FEE10000ull), n += 12;
    }
    return make_table(MADT_AT, "APIC", body, n);
}

static void make_hpet(uint8_t space)
{
    uint8_t body[20];

    memset(body, 0, sizeof body);
    put32(body, 0x8086A201u); /* event timer block id */
    body[4] = space;          /* address space */
    put64(body + 8, 0xFED00000u);
    body[16] = 0; /* HPET number */
    make_table(HPET_AT, "HPET", body, sizeof body);
}

static void make_roots(void)
{
    uint8_t rsdt[12];
    uint8_t xsdt[24];

    put32(rsdt, FACP_AT);
    put32(rsdt + 4, MADT_AT);
    put32(rsdt + 8, HPET_AT);
    make_table(RSDT_AT, "RSDT", rsdt, sizeof rsdt);
    put64(xsdt, FACP_AT);
    put64(xsdt + 8, MADT_AT);
    put64(xsdt + 16, HPET_AT);
    make_table(XSDT_AT, "XSDT", xsdt, sizeof xsdt);
    {
        const uint8_t facp[8] = {0};

        make_table(FACP_AT, "FACP", facp, sizeof facp);
    }
}

static void setup(bool lapic_override)
{
    memset(mem, 0, sizeof mem);
    make_roots();
    make_madt(lapic_override);
    make_hpet(0);
}

SMY_TEST(acpi_rsdt_finds_and_parses_madt)
{
    struct acpi a;
    struct acpi_madt_info m;
    uint16_t flags = 0xFFFF;

    setup(false);
    make_rsdp(0, RSDT_AT, 0);
    SMY_EXPECT_EQ_U64(acpi_init(&a, &ops, RSDP_AT), ACPI_OK);
    SMY_EXPECT_EQ_U64(a.revision, 0);
    SMY_EXPECT_EQ_U64(a.table_count, 3);
    SMY_EXPECT(strcmp(a.oem_id, "SMYOS ") == 0);
    SMY_EXPECT_EQ_U64(acpi_parse_madt(acpi_find_table(&a, "APIC", 0), &m), ACPI_OK);
    SMY_EXPECT_EQ_U64(m.lapic_address, 0xFEE00000u);
    SMY_EXPECT(m.has_legacy_pics);
    SMY_EXPECT_EQ_U64(m.cpu_count, 2);
    SMY_EXPECT_EQ_U64(m.ioapic_count, 1);
    SMY_EXPECT_EQ_U64(m.ioapics[0].address, 0xFEC00000u);
    SMY_EXPECT_EQ_U64(m.override_count, 2);
    SMY_EXPECT_EQ_U64(acpi_isa_irq_to_gsi(&m, 0, &flags), 2);
    SMY_EXPECT_EQ_U64(flags, 0);
    SMY_EXPECT_EQ_U64(acpi_isa_irq_to_gsi(&m, 1, &flags), 1); /* keyboard: identity */
    SMY_EXPECT_EQ_U64(acpi_isa_irq_to_gsi(&m, 9, &flags), 9);
    SMY_EXPECT_EQ_U64(flags & ACPI_INTI_POLARITY_MASK, ACPI_INTI_POLARITY_LOW);
    SMY_EXPECT_EQ_U64(flags & ACPI_INTI_TRIGGER_MASK, ACPI_INTI_TRIGGER_LEVEL);
    SMY_EXPECT(acpi_find_table(&a, "APIC", 1) == NULL);
    SMY_EXPECT(acpi_find_table(&a, "SSDT", 0) == NULL);
}

SMY_TEST(acpi_prefers_xsdt_and_honours_lapic_override)
{
    struct acpi a;
    struct acpi_madt_info m;
    struct acpi_hpet_info h;

    setup(true);
    /* RSDT pointer deliberately broken: revision 2 must use the XSDT. */
    make_rsdp(2, 0xDEAD0, XSDT_AT);
    SMY_EXPECT_EQ_U64(acpi_init(&a, &ops, RSDP_AT), ACPI_OK);
    SMY_EXPECT_EQ_U64(a.revision, 2);
    SMY_EXPECT_EQ_U64(acpi_parse_madt(acpi_find_table(&a, "APIC", 0), &m), ACPI_OK);
    SMY_EXPECT_EQ_U64(m.lapic_address, 0xFEE10000u);
    SMY_EXPECT_EQ_U64(acpi_parse_hpet(acpi_find_table(&a, "HPET", 0), &h), ACPI_OK);
    SMY_EXPECT_EQ_U64(h.address, 0xFED00000u);
}

SMY_TEST(acpi_rejects_bad_rsdp_and_root)
{
    struct acpi a;

    setup(false);
    SMY_EXPECT_EQ_U64(acpi_init(&a, &ops, 0), ACPI_ERR_NO_RSDP);
    SMY_EXPECT_EQ_U64(acpi_init(&a, &ops, MEM_SIZE), ACPI_ERR_NO_RSDP); /* unmappable */
    make_rsdp(0, RSDT_AT, 0);
    mem[RSDP_AT + 9] ^= 1; /* OEM byte changed, checksum now wrong */
    SMY_EXPECT_EQ_U64(acpi_init(&a, &ops, RSDP_AT), ACPI_ERR_BAD_RSDP);
    make_rsdp(2, RSDT_AT, XSDT_AT);
    mem[RSDP_AT + 30] ^= 1; /* extended checksum wrong */
    SMY_EXPECT_EQ_U64(acpi_init(&a, &ops, RSDP_AT), ACPI_ERR_BAD_RSDP);
    make_rsdp(0, RSDT_AT, 0);
    mem[RSDT_AT + 36] ^= 1; /* RSDT entry changed: checksum wrong */
    SMY_EXPECT_EQ_U64(acpi_init(&a, &ops, RSDP_AT), ACPI_ERR_BAD_ROOT);
    make_rsdp(0, MADT_AT, 0); /* root is not an RSDT */
    SMY_EXPECT_EQ_U64(acpi_init(&a, &ops, RSDP_AT), ACPI_ERR_BAD_ROOT);
}

SMY_TEST(acpi_skips_tables_with_bad_checksum_or_length)
{
    struct acpi a;

    setup(false);
    make_rsdp(0, RSDT_AT, 0);
    SMY_EXPECT_EQ_U64(acpi_init(&a, &ops, RSDP_AT), ACPI_OK);
    mem[MADT_AT + 40] ^= 0x80;
    SMY_EXPECT(acpi_find_table(&a, "APIC", 0) == NULL);
    setup(false);
    put32(mem + HPET_AT + 4, 0x7FFFFFFFu); /* absurd length */
    SMY_EXPECT(acpi_find_table(&a, "HPET", 0) == NULL);
    SMY_EXPECT(acpi_find_table(&a, "FACP", 0) != NULL);
}

SMY_TEST(acpi_rejects_malformed_madt_and_hpet)
{
    struct acpi_madt_info m;
    struct acpi_hpet_info h;
    const uint32_t len = (setup(false), make_madt(false));
    const struct acpi_sdt_header *madt = (const struct acpi_sdt_header *)(mem + MADT_AT);

    /* zero-length entry would loop forever */
    mem[MADT_AT + 44 + 1] = 0;
    SMY_EXPECT_EQ_U64(acpi_parse_madt(madt, &m), ACPI_ERR_BAD_TABLE);
    /* entry running past the end */
    make_madt(false);
    mem[MADT_AT + len - 6 + 1] = 40;
    SMY_EXPECT_EQ_U64(acpi_parse_madt(madt, &m), ACPI_ERR_BAD_TABLE);
    /* no I/O APIC */
    {
        const uint8_t body[8] = {0, 0, 0xE0, 0xFE, 0, 0, 0, 0};

        make_table(MADT_AT, "APIC", body, sizeof body);
        SMY_EXPECT_EQ_U64(acpi_parse_madt(madt, &m), ACPI_ERR_BAD_TABLE);
    }
    SMY_EXPECT_EQ_U64(acpi_parse_madt(NULL, &m), ACPI_ERR_BAD_TABLE);
    make_hpet(1); /* I/O space: not usable */
    SMY_EXPECT_EQ_U64(acpi_parse_hpet((struct acpi_sdt_header *)(mem + HPET_AT), &h),
                      ACPI_ERR_BAD_TABLE);
    SMY_EXPECT(strcmp(acpi_status_name(ACPI_ERR_BAD_ROOT), "invalid RSDT/XSDT") == 0);
}
