/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

/* Host tests for kernel/arch/x86_64/descriptors.c against known encodings. */

#include "arch/x86_64/descriptors.h"
#include "harness/smytest.h"

SMY_TEST(gdt_segment_encodes_standard_flat_segments)
{
    /* Well-known values for flat 4 GiB segments in long mode. */
    SMY_EXPECT_EQ_U64(gdt_segment(0, 0xFFFFF, GDT_ACCESS_KERNEL_CODE, GDT_FLAGS_CODE64),
                      0x00AF9A000000FFFFu);
    SMY_EXPECT_EQ_U64(gdt_segment(0, 0xFFFFF, GDT_ACCESS_KERNEL_DATA, GDT_FLAGS_DATA),
                      0x00CF92000000FFFFu);
    SMY_EXPECT_EQ_U64(gdt_segment(0, 0xFFFFF, GDT_ACCESS_USER_CODE, GDT_FLAGS_CODE64),
                      0x00AFFA000000FFFFu);
    SMY_EXPECT_EQ_U64(gdt_segment(0, 0xFFFFF, GDT_ACCESS_USER_DATA, GDT_FLAGS_DATA),
                      0x00CFF2000000FFFFu);
}

SMY_TEST(gdt_segment_splits_base_and_limit)
{
    /* byte 7 = base[31:24] 0x12, byte 6 = flags 0x4 | limit[19:16] 0xA,
     * byte 5 = access 0x92, bytes 4..2 = base[23:0], bytes 1..0 = limit[15:0] */
    SMY_EXPECT_EQ_U64(gdt_segment(0x12345678, 0xABCDE, 0x92, 0x4), 0x124A92345678BCDEu);
}

SMY_TEST(gdt_tss_descriptor_spans_two_slots)
{
    uint64_t d[2];

    gdt_tss_descriptor(d, 0xFFFFFFFF80012345u, 103);
    /* limit 103, base[23:0]=0x012345, access 0x89, base[31:24]=0x80 */
    SMY_EXPECT_EQ_U64(d[0], 0x8000890123450067u);
    SMY_EXPECT_EQ_U64(d[1], 0xFFFFFFFFu);
}

SMY_TEST(idt_gate_round_trips_handler_and_fields)
{
    const uint64_t handler = 0xFFFFFFFF80001234u;
    const struct idt_gate g =
        idt_make_gate(handler, GDT_KERNEL_CODE, IST_DOUBLE_FAULT, IDT_GATE_INTERRUPT_DPL0);

    SMY_EXPECT_EQ_U64(g.offset_low, 0x1234);
    SMY_EXPECT_EQ_U64(g.offset_mid, 0x8000);
    SMY_EXPECT_EQ_U64(g.offset_high, 0xFFFFFFFFu);
    SMY_EXPECT_EQ_U64(g.selector, 0x08);
    SMY_EXPECT_EQ_U64(g.ist, 1);
    SMY_EXPECT_EQ_U64(g.type_attr, 0x8E);
    SMY_EXPECT_EQ_U64(g.reserved, 0);
    SMY_EXPECT_EQ_U64(idt_gate_handler(&g), handler);
    /* IST is a 3-bit field. */
    SMY_EXPECT_EQ_U64(idt_make_gate(handler, 0x08, 9, 0x8E).ist, 1);
}
