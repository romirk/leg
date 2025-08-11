//
// Created by Romir Kulshrestha on 08/06/2025.
//

#include "memory.h"
#include "cpu.h"

#define MB_SHIFT 20

[[gnu::section(".tt")]]
[[gnu::aligned(0x4000)]]
l1_table_entry l1_translation_table[0x1000];

void init_mmu(void) {
    // identity map ram
    for (int i = 0x400; i < 0xc00; i++)
        l1_translation_table[i] = (l1_table_entry){
            .section = {
                .address = i,
                .access_perms = 0b10,
                .type_ext = 0b001,
                .bufferable = true,
                .cacheable = true,
                .type = 0b10,
            }
        };

    // UART
    l1_translation_table[0x090] = (l1_table_entry){
        .section = {
            .address = 0x090,
            .access_perms = 0b10,
            .type_ext = 0b000,
            .bufferable = false,
            .cacheable = false,
            .type = 0b10,
        }
    };

    asm volatile (
        "mcr p15, 0, %0, c2, c0, 0;" // TTBR0
        "mcr p15, 0, %0, c2, c0, 1" // TTBR1
        :: "r" ((void *) l1_translation_table));

    // Prep MMU
    asm volatile ("mcr p15, 0, %0, c3, c0, 0" :: "r" (1)); // Domain Access Control Register
    // Flush data before the MMU is enabled.
    asm volatile (
        "mcr p15, 0, %0, c2, c0, 2;" // TTBCR, N = 0
        "mcr p15, 0, %0, c7,  c7, 0;" // Invalidate instruction and data caches
        "mcr p15, 0, %0, c8,  c7, 0;" // Invalidates the TLB
        "dsb;" // Data Synchronization Barrier
        "isb;" // Instruction Synchronization Barrier
        :: "r" (0));

    // Enable MMU
    struct sctlr sctlr;
    asm ("mrc p15, 0, %0, c1, c0, 0" : "=r"(sctlr));
    sctlr.M = true;
    asm ("mcr p15, 0, %0, c1, c0, 0" :: "r"(sctlr));
}
