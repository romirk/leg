//
// Created by Romir Kulshrestha on 09/05/2026.
//

#include "kernel/arch/aarch64/tt.h"
#include "kernel/arch/aarch64/memory.h"

#define MB2    (2 * MB)
#define AS_2MB (AS_MB + 1)

[[gnu::section(".tt"), gnu::aligned(0x1000)]]
l1_descriptor tt_l1[512] = {
    [0] = {.fields = {.descriptor = PTE_DESC_TABLE}},
    [1] = {.fields = {.descriptor = PTE_DESC_TABLE}},
};

[[gnu::section(".tt"), gnu::aligned(0x1000)]]
l2_descriptor tt_l2_dev[512] = {
    [64] =
        {
            .fields =
                {
                    .descriptor = PTE_DESC_BLOCK,
                    .out_pa     = GICD_PHYSICAL >> AS_2MB,
                    .attr_indx  = 1,
                    .ap         = 0b00,
                    .sh         = 0b11,
                    .af         = 1,
                    .ng         = 0,
                },
        },
    [72] =
        {
            .fields =
                {
                    .descriptor = PTE_DESC_BLOCK,
                    .out_pa     = UART0_PHYSICAL >> AS_2MB,
                    .attr_indx  = 1,
                    .ap         = 0b00,
                    .sh         = 0b11,
                    .af         = 1,
                    .ng         = 0,
                },
        },
};

[[gnu::section(".tt"), gnu::aligned(0x1000)]]
l2_descriptor tt_l2_ram[512] = {0};

void init_pgtables() {
    tt_l1[0].raw = PTE_DESC_TABLE | (uptr) tt_l2_dev;
    tt_l1[1].raw = PTE_DESC_TABLE | (uptr) tt_l2_ram;

    for (int i = 0; i < 512; i++) {
        tt_l2_ram[i] = (l2_descriptor) {.fields = {
                                               .descriptor = PTE_DESC_BLOCK,
                                               .out_pa     = (RAM_BASE + i * MB2) >> AS_2MB,
                                               .attr_indx  = 0,
                                               .ap         = 0b00,
                                               .sh         = 0b11,
                                               .af         = 1,
                                               .ng         = 0,
                                           }};
    }
}
