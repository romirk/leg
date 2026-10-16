//
// Created by Romir Kulshrestha on 08/05/2026.
//

#pragma once

#include "types.h"

typedef enum : u8 {
    PTE_DESC_INVALID = 0b00,
    PTE_DESC_BLOCK   = 0b01,
    PTE_DESC_TABLE   = 0b11, // L1/L2: next-level table
    PTE_DESC_PAGE    = 0b11, // L3: 4 KB page (0b01 is reserved at L3)
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
        u8 : 5;
        u8 pxn : 1;
        u8 uxn : 1;
    } fields;
} l2_descriptor;

// L3 page descriptor (4 KB granule): same attributes as an L2 block, output address at [47:12].
typedef union {
    u64 raw;
    struct [[gnu::packed, gnu::aligned(8)]] {
        pte_desc descriptor : 2; // [1:0]   PTE_DESC_PAGE
        u8       attr_indx : 3;  // [4:2]
        u8 : 1;                  // [5]     NS
        u8  ap : 2;              // [7:6]
        u8  sh : 2;              // [9:8]
        u8  af : 1;              // [10]
        u8  ng : 1;              // [11]
        u64 out_pa : 36;         // [47:12] PA >> 12
        u8 : 5;                  // [52:48]
        u8 pxn : 1;              // [53]
        u8 uxn : 1;              // [54]
        u16 : 9;                 // [63:55]
    } fields;
} l3_descriptor;
static_assert(sizeof(l3_descriptor) == 8);

extern l1_descriptor tt_l1[512];

void init_pgtables();
