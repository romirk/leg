//
// Created by Romir Kulshrestha on 08/06/2025.
//

#include "kernel/mmu.h"
#include "kernel/cpu.h"
#include "kernel/linker.h"
#include "utils.h"

#define MB_SHIFT 20
#define KPHYS_OFFSET 0x200000  // kernel placed 2MB past DTB section base

u32 kernel_phys_base;

[[gnu::section(".tt")]]
[[gnu::aligned(0x4000)]]
translation_table kernel_translation_table;

[[gnu::section(".tt")]]
page_table kernel_page_table;

[[gnu::section(".tt")]]
page_table peripheral_page_table;

[[gnu::section(".tt")]]
page_table process_page_tables[8];

// offset of a kernel virtual symbol within the kernel image
#define KRN_OFFSET(sym) ((u32)(sym) - (u32)kernel_main_beg)

[[gnu::section(".startup.mmu")]]
static void map_sections(void *dtb) {
    const u32 kphys = ((u32)dtb & ~0xFFFFF) + KPHYS_OFFSET;
    const auto table = (l1_entry *)(kphys + KRN_OFFSET(kernel_translation_table));

    // kernel section (flash) — identity map ROM so kboot can continue
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

    // kernel section (virtual 0xC00 → physical kphys)
    table[(u32)kernel_main_beg >> MB_SHIFT] = (l1_entry){
        .section = {
            .type = L1_SECTION,
            .address = kphys >> MB_SHIFT,
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
    map_sections(dtb);

    const u32 kphys = ((u32)dtb & ~0xFFFFF) + KPHYS_OFFSET;
    auto table = (l1_entry *)(kphys + KRN_OFFSET(kernel_translation_table));
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
