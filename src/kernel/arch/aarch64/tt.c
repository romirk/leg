//
// Created by Romir Kulshrestha on 09/05/2026.
//

#include "kernel/arch/aarch64/tt.h"
#include "kernel/arch/aarch64/memory.h"
#include "kernel/arch/aarch64/linker.h"

#define L2_PA(addr) ((addr) >> 21)
#define L3_PA(addr) ((addr) >> 12)

#define MAP(level, addr, ...)                                                                      \
    [L##level##_PA(addr) & 0x1ff] = (l##level##_descriptor) {                                      \
        .fields = __VA_ARGS__, .fields.out_pa = L##level##_PA(addr)                                \
    }

[[gnu::section(".tt"), gnu::aligned(0x1000)]]
l1_descriptor tt_l1[512] = {
    [0] = {.fields = {.descriptor = PTE_DESC_TABLE}},
    [1] = {.fields = {.descriptor = PTE_DESC_TABLE}},
};

[[gnu::section(".tt"), gnu::aligned(0x1000)]]
static l2_descriptor tt_l2_dev[512] = {
    MAP(2, GICD_PHYSICAL,
        {
            .descriptor = PTE_DESC_BLOCK,
            .attr_indx  = 1,
            .ap         = 0b00,
            .sh         = 0b11,
            .af         = 1,
            .ng         = 0,
            .pxn        = 1,
            .uxn        = 1,
        }),
    MAP(2, UART0_PHYSICAL,
        {
            .descriptor = PTE_DESC_BLOCK,
            .attr_indx  = 1,
            .ap         = 0b00,
            .sh         = 0b11,
            .af         = 1,
            .ng         = 0,
            .pxn        = 1,
            .uxn        = 1,
        }),
};

[[gnu::section(".tt"), gnu::aligned(0x1000)]]
static l2_descriptor tt_l2_ram[512] = {0};

[[gnu::section(".tt"), gnu::aligned(0x1000)]]
static l3_descriptor tt_l3_krn[512] = {0};

static void map_n_pages_l2(l2_descriptor table[], const uptr base, const u16 n,
                           const l2_descriptor template) {
    constexpr uptr page_size = 2 * MB;
    const auto     fields    = &template.fields;
    for (auto addr = base; addr < base + n * page_size; addr += page_size)
        table MAP(2, addr,
                  {
                      .descriptor = fields->descriptor,
                      .attr_indx  = fields->attr_indx,
                      .pxn        = fields->pxn,
                      .uxn        = fields->uxn,
                      .af         = fields->af,
                      .ng         = fields->ng,
                      .ap         = fields->ap,
                      .sh         = fields->sh,
                  });
}

static void map_n_pages_l3(l3_descriptor table[], const uptr base, const u16 n,
                           const l3_descriptor template) {
    constexpr uptr page_size = 4 * KB;
    const auto     fields    = &template.fields;
    for (auto addr = base; addr < base + n * page_size; addr += page_size)
        table MAP(3, addr,
                  {
                      .descriptor = fields->descriptor,
                      .attr_indx  = fields->attr_indx,
                      .pxn        = fields->pxn,
                      .uxn        = fields->uxn,
                      .sh         = fields->sh,
                      .af         = fields->af,
                      .ng         = fields->ng,
                      .ap         = fields->ap,
                  });
}

static void map_section(l3_descriptor table[], const uptr beg, const uptr end,
                        const l3_descriptor template) {
    constexpr uptr page_size = 4 * KB;
    const auto     n_pages   = (end - beg + page_size - 1) / page_size;
    map_n_pages_l3(table, beg, n_pages, template);
}

void init_pgtables() {
    tt_l1[0].raw = PTE_DESC_TABLE | (uptr) tt_l2_dev;
    tt_l1[1].raw = PTE_DESC_TABLE | (uptr) tt_l2_ram;

    map_n_pages_l2(tt_l2_ram, RAM_BASE, 512,
                   (l2_descriptor) {.fields = {
                                        .descriptor = PTE_DESC_BLOCK,
                                        .attr_indx  = 0,
                                        .ap         = 0b00,
                                        .sh         = 0b11,
                                        .af         = 1,
                                        .ng         = 0,
                                        .pxn        = 1,
                                        .uxn        = 1,
                                    }});

    tt_l2_ram[L2_PA(KERNEL_PA) & 0x1ff].raw = PTE_DESC_TABLE | (uptr) tt_l3_krn;

    map_section(tt_l3_krn, (uptr) SEC_TEXT_BEG, (uptr) SEC_TEXT_END,
                (l3_descriptor) {.fields = {
                                     .descriptor = PTE_DESC_PAGE,
                                     .attr_indx  = 0,
                                     .sh         = 0b11,
                                     .af         = 1,
                                     .ng         = 0,
                                     .ap         = 0b10,
                                     .pxn        = 0,
                                     .uxn        = 1,
                                 }});

    map_section(tt_l3_krn, (uptr) SEC_RODATA_BEG, (uptr) SEC_RODATA_END,
                (l3_descriptor) {.fields = {
                                     .descriptor = PTE_DESC_PAGE,
                                     .attr_indx  = 0,
                                     .sh         = 0b11,
                                     .af         = 1,
                                     .ng         = 0,
                                     .ap         = 0b10,
                                     .pxn        = 1,
                                     .uxn        = 1,
                                 }});

    map_section(tt_l3_krn, (uptr) SEC_DATA_BEG, (uptr) SEC_DATA_END,
                (l3_descriptor) {.fields = {
                                     .descriptor = PTE_DESC_PAGE,
                                     .attr_indx  = 0,
                                     .sh         = 0b11,
                                     .af         = 1,
                                     .ng         = 0,
                                     .ap         = 0b00,
                                     .pxn        = 1,
                                     .uxn        = 1,
                                 }});

    map_section(tt_l3_krn, (uptr) SEC_BSS_BEG, (uptr) SEC_BSS_END,
                (l3_descriptor) {.fields = {
                                     .descriptor = PTE_DESC_PAGE,
                                     .attr_indx  = 0,
                                     .sh         = 0b11,
                                     .af         = 1,
                                     .ng         = 0,
                                     .ap         = 0b00,
                                     .pxn        = 1,
                                     .uxn        = 1,
                                 }});
    map_section(tt_l3_krn, (uptr) SEC_TT_BEG, (uptr) SEC_TT_END,
                (l3_descriptor) {.fields = {
                                     .descriptor = PTE_DESC_PAGE,
                                     .attr_indx  = 0,
                                     .sh         = 0b11,
                                     .af         = 1,
                                     .ng         = 0,
                                     .ap         = 0b00,
                                     .pxn        = 1,
                                     .uxn        = 1,
                                 }});

    // Rest of the kernel's 2 MB block after the image (.bss is last): plain RAM, RW + XN.
    const uptr image_end = ((uptr) SEC_BSS_END + 4 * KB - 1) & ~(4 * KB - 1);
    const uptr block_end = (KERNEL_PA & ~(2 * MB - 1)) + 2 * MB;
    map_section(tt_l3_krn, image_end, block_end,
                (l3_descriptor) {.fields = {
                                     .descriptor = PTE_DESC_PAGE,
                                     .attr_indx  = 0,
                                     .sh         = 0b11,
                                     .af         = 1,
                                     .ng         = 0,
                                     .ap         = 0b00,
                                     .pxn        = 1,
                                     .uxn        = 1,
                                 }});
}
