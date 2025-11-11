//
// Created by Romir Kulshrestha on 08/06/2025.
//

#include "memory.h"
#include "cpu.h"
#include "linker.h"
#include "utils.h"
#include "fdt/fdt.h"

#define MB_SHIFT 20

[[gnu::section(".tt")]]
[[gnu::aligned(0x4000)]]
translation_table kernel_translation_table;

[[gnu::section(".tt")]]
page_table kernel_page_table;

[[gnu::section(".tt")]]
page_table peripheral_page_table;

[[gnu::section(".tt")]]
page_table dtb_page_table;

[[gnu::section(".tt")]]
page_table process_page_tables[8];

[[gnu::section(".startup.mmu")]]
static void map_sections() {
    auto table = (l1_entry *) ((u32) kernel_translation_table - VIRTUAL_OFFSET);
    auto dtb_table = (l2_entry *) ((u32) dtb_page_table - VIRTUAL_OFFSET);

    // kernel section (flash)
    table[0x000] = (l1_entry){
        .section = {
            .type = L1_SECTION,
            .address = 0x000,
            .access_perms = 0b10,
            .type_ext = 0b001,
            .bufferable = true,
            .cacheable = true,
        }
    };

    // kernel section (virtual)
    table[get_high_bits(kernel_main_beg, 12)] = (l1_entry){
        .section = {
            .type = L1_SECTION,
            .address = get_high_bits((u32) kernel_main_beg - VIRTUAL_OFFSET, 12),
            .access_perms = 0b10,
            .type_ext = 0b001,
            .bufferable = true,
            .cacheable = true,
        }
    };

    // UART
    table[0x090] = (l1_entry){
        .section = {
            .type = L1_SECTION,
            .address = 0x090,
            .access_perms = 0b10,
            .type_ext = 0b000,
            .bufferable = true,
            .cacheable = false,
        }
    };

    // DTB section
    table[0x400] = (l1_entry){
        .page_table = {
            .type = L1_PAGE_TABLE,
            .address = get_high_bits(dtb_table, 22),
        }
    };

    for (int i = 0; i < 16; ++i) {
        dtb_table[i] = (l2_entry){
            .large_page = {
                .type = L2_LARGE_PAGE,
                .bufferable = true,
                .cacheable = true,
                .access_perms = 0b10,
                .type_ext = 0b001,
                .address = get_high_bits(FDT_ADDR, 16),
            }
        };
    }
}

[[gnu::section(".startup.mmu")]]
void init_mmu(void) {
    // manually map sections before mmu starts
    map_sections();

    auto table = (l1_entry *) ((u32) kernel_translation_table - VIRTUAL_OFFSET);
    asm (
        "mcr p15, 0, %0, c2, c0, 0;" // TTBR0
        "mcr p15, 0, %0, c2, c0, 1" // TTBR1
        :: "r" ((void *) table));

    // Prep MMU
    asm ("mcr p15, 0, %0, c3, c0, 0" :: "r" (1)); // Domain Access Control Register

    asm ("mcr p15, 0, %0, c2, c0, 2" :: "r" (0));

    // Flush data before the MMU is enabled.
    asm (
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

    // we are now in virtual address space
}
