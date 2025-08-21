//
// Created by Romir Kulshrestha on 08/06/2025.
//

#ifndef MEMORY_H
#define MEMORY_H
#include "types.h"

#define VIRTUAL_OFFSET 0x40000000

typedef enum {
    L1_INVALID = 0b00,
    L1_PAGE_TABLE = 0b01,
    L1_SECTION = 0b10,
    L1_PXN_SECTION = 0b11
} l1_entry_type;

typedef enum {
    L2_INVALID = 0b00,
    L2_LARGE_PAGE = 0b01,
    L2_SMALL_PAGE = 0b10,
    // small page with XN set
    L2_XN_PAGE = 0b11
} l2_entry_type;

typedef union {
    // representation of entry in memory
    u32 raw;

    // bits [0:1] specifying entry type
    l1_entry_type type: 2;

    struct [[gnu::packed]] [[gnu::aligned(4)]] {
        l1_entry_type type: 2;
        bool privileged_execute_never: 1;
        bool non_secure: 1;
        bool : 1;
        u8 domain: 4;
        bool : 1;

        // page table base address [31:10]
        u32 address: 22;
    } page_table;

    struct [[gnu::packed]] [[gnu::aligned(4)]] {
        l1_entry_type type: 2;
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

    struct [[gnu::packed]] [[gnu::aligned(4)]] {
        l1_entry_type type: 2;
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
    l2_entry_type type: 2;

    struct [[gnu::packed]] [[gnu::aligned(4)]] {
        l2_entry_type type: 2;
        bool bufferable: 1;
        bool cacheable: 1;
        u8 access_perms: 2;
        u8 : 3;
        bool access_ext: 1;
        bool shareable: 1;
        bool not_global: 1;
        u8 type_ext: 3;
        bool execute_never: 1;

        // large page base address [31:16]
        u16 address;
    } large_page;

    struct [[gnu::packed]] [[gnu::aligned(4)]] {
        l2_entry_type type: 2;
        bool bufferable: 1;
        bool cacheable: 1;
        u8 access_perms: 2;
        u8 type_ext: 3;
        bool access_ext: 1;
        bool shareable: 1;
        bool not_global: 1;

        // small page base address [31:12]
        u32 address: 20;
    } small_page;
} l2_table_entry;

/// L1 translation table
typedef l1_table_entry translation_table[0x1000];
/// L2 translation table
typedef l2_table_entry page_table[256];

/**
 * Configures the MMU and L1 translation table, ID-mapping the first mib of flash, RAM, and the UART address spaces.
 */
void init_mmu(void);

extern translation_table kernel_translation_table;

#endif //MEMORY_H
