//
// Created by Romir Kulshrestha on 08/06/2025.
//

#ifndef MEMORY_H
#define MEMORY_H
#include "types.h"

typedef union {
    // representation of entry in memory
    u32 raw;

    // bits [0:1] specifying entry type
    u8 type: 2;

    struct [[gnu::packed]] {
        u8 type: 2;
        bool PXN: 1;
        bool NS: 1;
        bool : 1;
        u8 domain: 4;
        bool : 1;

        // page table base address [31:10]
        u32 address: 22;
    } page_table;

    struct [[gnu::packed]] {
        u8 type: 2;
        bool bufferable: 1;
        bool cacheable: 1;
        bool execute_never: 1;
        u8 domain: 4;
        bool : 1;
        u8 access_perms: 2;
        u8 type_ext: 3;
        bool access_ext: 1;
        bool shareable: 1;
        bool not_global: 1;
        bool supersection: 1;
        bool non_secure: 1;

        // section base address [31:20]
        u32 address: 12;
    } section;

    struct [[gnu::packed]] {
        u8 type: 2;
        bool bufferable: 1;
        bool cacheable: 1;
        bool execute_never: 1;

        // extended address [39:36]
        u8 xaddr_39_36: 4;

        bool : 1;
        u8 access_perms: 2;
        u8 type_ext: 3;
        bool access_ext: 1;
        bool shareable: 1;
        bool not_global: 1;
        bool supersection: 1;
        bool non_secure: 1;

        // extended address [35:32]
        u8 xaddr_35_32: 4;

        // supersection base address [31:24]
        u8 address;
    } supersection;
} l1_table_entry;

typedef union {
    // representation of entry in memory
    u32 raw;

    // bits [0:1] specifying entry type
    u8 type: 2;

    struct [[gnu::packed]] {
        u8 type: 2;
        bool B: 1;
        bool C: 1;
        u8 AP10: 2;
        u8 : 3;
        bool AP2: 1;
        bool S: 1;
        bool nG: 1;
        u8 TEX: 3;
        bool XN: 1;

        // large page base address [31:16]
        u16 address;
    } large_page;

    struct [[gnu::packed]] {
        bool XN: 1;
        bool : 1;
        bool B: 1;
        bool C: 1;
        u8 AP10: 2;
        u8 : 3;
        bool AP2: 1;
        bool S: 1;
        bool nG: 1;

        // small page base address [31:12]
        u32 address: 20;
    } small_page;
} l2_table_entry;

void init_mmu(void);

extern l1_table_entry l1_translation_table[0x1000];

#endif //MEMORY_H