//
// Created by Romir Kulshrestha on 08/06/2025.
//

#include "memory.h"
#include "cpu.h"
#include "utils.h"

#define MB_SHIFT 20

[[gnu::section(".tt")]]
[[gnu::aligned(0x4000)]]
translation_table kernel_translation_table;

[[gnu::section(".tt")]]
page_table kernel_page_table;

[[gnu::section(".tt")]]
page_table peripheral_page_table;

[[gnu::section(".tt")]]
page_table process_page_tables[8];


[[gnu::section(".startup.mmu")]]
void init_mmu(void) {
    // manually map sections before mmu starts
    l1_table_entry *l1_table = kernel_translation_table - 0x10000000;
    // l2_table_entry *l2_table = kernel_page_table - 0x10000000;

    // translation table
    l1_table[0xfff] = (l1_table_entry){
        .section = {
            .type = L1_SECTION,
            .address = get_high_bits(l1_table, 12),
            .access_perms = 0b10,
            .type_ext = 0b000,
            .bufferable = true,
            .cacheable = false,
        }
    };

    // kernel section (flash)
    l1_table[0x000] = (l1_table_entry){
        .section = {
            .type = L1_SECTION,
            .address = 0x000,
            .access_perms = 0b10,
            .type_ext = 0b001,
            .bufferable = true,
            .cacheable = true,
        }
    };

    // kernel section (RAM)
    l1_table[0x400] = (l1_table_entry){
        .section = {
            .type = L1_SECTION,
            .address = get_high_bits(0x40000000, 12),
            .access_perms = 0b10,
            .type_ext = 0b001,
            .bufferable = true,
            .cacheable = true,
        }
    };

    asm volatile (
        "mcr p15, 0, %0, c2, c0, 0;" // TTBR0
        "mcr p15, 0, %0, c2, c0, 1" // TTBR1
        :: "r" ((void *) l1_table));

    // Prep MMU
    asm volatile ("mcr p15, 0, %0, c3, c0, 0" :: "r" (1)); // Domain Access Control Register

    asm volatile ("mcr p15, 0, %0, c2, c0, 2" :: "r" (0));

    // Flush data before the MMU is enabled.
    asm volatile (
        "mcr p15, 0, %0, c7,  c7, 0;" // Invalidate instruction and data caches
        "mcr p15, 0, %0, c8,  c7, 0;" // Invalidates the TLB
        "dsb;" // Data Synchronization Barrier
        "isb;" // Instruction Synchronization Barrier
        :: "r" (0));

    // Enable MMU
    struct sctlr sctlr;
    asm ("mrc p15, 0, %0, c1, c0, 0" : "=r"(sctlr));
    sctlr.M = true;
    asm ("mcr p15, 0, %0, c1, c0, 0" :: "r"(sctlr) : "memory");

    // UART
    kernel_translation_table[0x090] = (l1_table_entry){
        .section = {
            .type = L1_SECTION,
            .address = 0x090,
            .access_perms = 0b10,
            .type_ext = 0b000,
            .bufferable = true,
            .cacheable = false,
        }
    };

    // peripherals
    kernel_translation_table[0xffe] = (l1_table_entry){
        .page_table = {
            .type = L1_PAGE_TABLE,
            .address = get_high_bits(peripheral_page_table, 22),
        }
    };

    // UART
    peripheral_page_table[0x00] = (l2_table_entry){
        .small_page = {
            .type = L2_SMALL_PAGE,
            .address = get_high_bits(peripheral_page_table, 20),
            .bufferable = true,
            .cacheable = false,
            .access_perms = 0b10,
            .type_ext = 0b000,
        }
    };
}
