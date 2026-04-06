//
// Created by Romir Kulshrestha on 08/06/2025.
//

#include "kernel/mmu.h"
#include "kernel/cpu.h"
#include "kernel/linker.h"
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
static void map_sections(void *dtb) {
    const auto table = (l1_entry *) ((u32) kernel_translation_table - VIRTUAL_OFFSET);

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

    // GIC
    table[0x080] = (l1_entry){
        .section = {
            .type = L1_SECTION,
            .address = 0x080,
            .access_perms = 0b10,
            .type_ext = 0b000,
            .bufferable = true,
            .cacheable = false,
        }
    };

    // DTB / early RAM — identity-map 2MB from DTB base (DTB can be up to 1MB + early heap)
    const u32 dtb_section = (u32) dtb >> MB_SHIFT;
    for (u32 i = 0; i < 2; ++i) {
        table[dtb_section + i] = (l1_entry){
            .section = {
                .type = L1_SECTION,
                .address = dtb_section + i,
                .access_perms = 0b10,
                .type_ext = 0b001,
                .bufferable = true,
                .cacheable = true,
            }
        };
    }
}

[[gnu::section(".startup.mmu")]]
void init_mmu(void *dtb) {
    // manually map sections before mmu starts
    map_sections(dtb);

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
