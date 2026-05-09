//
// Created by Romir Kulshrestha on 08/05/2026.
//

#ifndef LEG_TT_H
#define LEG_TT_H

#include "types.h"

typedef enum : u8 {
    PTE_DESC_INVALID = 0b00,
    PTE_DESC_BLOCK   = 0b01,
    PTE_DESC_TABLE   = 0b11
} pte_desc;

typedef union {
    u64 raw;
    struct [[gnu::packed, gnu::aligned(8)]] {
        pte_desc descriptor : 2;
        u16 : 10;
        u64 addr : 36;
    } fields;
} l1_descriptor;

typedef union {
    u64 raw;
    struct [[gnu::packed, gnu::aligned(8)]] {
        pte_desc descriptor : 2;
        u8       attr_indx : 3;
        u8 : 1;
        u8 ap : 2;
        u8 sh : 2;
        u8 af : 1;
        u8 ng : 1;
        u16 : 9;
        u32 out_pa : 27;
        u8 : 6;
        u8 pxn : 1;
        u8 uxn : 1;
    } fields;
} l2_descriptor;

extern l1_descriptor ttbr0_l1[512];
extern l2_descriptor ttbr0_l2_dev[512];
extern l2_descriptor ttbr0_l2_ram[512];
extern l1_descriptor ttbr1_l1[512];
extern l2_descriptor ttbr1_l2[512];

void init_pgtables();
void unmap_identity();

#endif // LEG_TT_H
