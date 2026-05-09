//
// Created by Romir Kulshrestha on 09/05/2026.
//

#include "kernel/arch/aarch64/tt.h"

[[gnu::section(".boot.tables"), gnu::aligned(0x1000)]]
l1_descriptor ttbr0_l1[512] = {
    [0] = {.fields = {.descriptor = PTE_DESC_TABLE}},
    [1] = {.fields = {.descriptor = PTE_DESC_TABLE}},
};

[[gnu::section(".boot.tables"), gnu::aligned(0x1000)]]
l2_descriptor ttbr0_l2_dev[512] = {
    [8] =
        {
            .fields =
                {
                    .descriptor = PTE_DESC_BLOCK,
                    .out_pa     = 0x09000000 >> 21,
                    .attr_indx  = 1,
                    .ap         = 0b00,
                    .sh         = 0b11,
                    .af         = 1,
                    .ng         = 0,
                },
        },
};

[[gnu::section(".boot.tables"), gnu::aligned(0x1000)]]
l2_descriptor ttbr0_l2_ram[512] = {
    [0] =
        {
            .fields =
                {
                    .descriptor = PTE_DESC_BLOCK,
                    .out_pa     = 0x40000000 >> 21,
                    .attr_indx  = 0,
                    .ap         = 0b00,
                    .sh         = 0b11,
                    .af         = 1,
                    .ng         = 0,
                },
        },
};

[[gnu::section(".boot.tables"), gnu::aligned(0x1000)]]
l1_descriptor ttbr1_l1[512] = {
    [0] = {.fields = {.descriptor = PTE_DESC_TABLE}},
};

[[gnu::section(".boot.tables"), gnu::aligned(0x1000)]]
l2_descriptor ttbr1_l2[512] = {
    [0] =
        {
            .fields =
                {
                    .descriptor = PTE_DESC_BLOCK,
                    .out_pa     = 0x40000000 >> 21,
                    .attr_indx  = 0,
                    .ap         = 0b00,
                    .sh         = 0b11,
                    .af         = 1,
                    .ng         = 0,
                },
        },
};

[[gnu::section(".boot")]]
void init_pgtables() {
    ttbr0_l1[0].fields.addr = (u64) ttbr0_l2_dev;
    ttbr0_l1[1].fields.addr = (u64) ttbr0_l2_ram;

    ttbr1_l1[0].fields.addr = (u64) ttbr1_l2;
}