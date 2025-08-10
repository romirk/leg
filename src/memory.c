//
// Created by Romir Kulshrestha on 08/06/2025.
//

#include "memory.h"

#include "cpu.h"
#include "linker.h"

#define MB_SHIFT 20

[[gnu::section(".startup.tt")]]
[[gnu::aligned(0x4000)]]
l1_table_entry l1_translation_table[0x1000] = {
    (l1_table_entry){
        .section = {
            .address = 0x000,
            .access_perms = 0b11,
            .type_ext = 0b001,
            .bufferable = true,
            .cacheable = true,
            .type = 0b10,
        }
    }
};

[[gnu::section(".startup.mmu")]]
void init_mmu(void) {
    l1_translation_table[0x400] = (l1_table_entry){
        .section = {
            .address = 0x400,
            .access_perms = 0b11,
            .type_ext = 0b001,
            .bufferable = true,
            .cacheable = true,
            .type = 0b10,
        }
    };

    // TTBR0
    asm volatile("MCR p15, 0, %0, c2, c0, 0" :: "r" ((void *) l1_translation_table));
    // TTBR1
    asm volatile("MCR p15, 0, %0, c2, c0, 1" :: "r" ((void *) l1_translation_table));

    // TTBCR
    asm volatile("MCR p15, 0, %0, c2, c0, 2" :: "r" (0)); // N = 0

    // Prep MMU
    // Flush data before the MMU is enabled.
    asm volatile (
        "mcr p15, 0, %0, c7,  c7, 0\n\t" // Invalidate instruction and data caches
        "mcr p15, 0, %0, c8,  c7, 0\n\t" // Invalidates the TLB
        "dsb" // Data Synchronization Barrier operation
        :: "r" (0));

    asm volatile ("mcr p15, 0, %0, c3, c0, 0" :: "r" (0xffffffff)); // Domain Access Control Register

    // Enable MMU
    struct sctlr sctlr;
    asm ("mrc p15, 0, %0, c1, c0, 0" : "=r"(sctlr));
    sctlr.M = true;
    asm ("mcr p15, 0, %0, c1, c0, 0" :: "r"(sctlr));
}
