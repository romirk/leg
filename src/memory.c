//
// Created by Romir Kulshrestha on 08/06/2025.
//

#include "memory.h"
#include "linker.h"

#define MB_SHIFT 20

__attribute__((aligned(0x4000))) unsigned int l1_translation_table[4096];

[[gnu::section(".startup.c")]]
[[clang::optnone]]
void init_translation_table(void) {
    l1_table_entry *l1_start = (void *) l1_translation_table;
    // constexpr auto l1_size = 4096 * sizeof(l1_table_entry);

    // clear table entries
    // memclr(l1_start, l1_size);

    for (unsigned int va = 0x00000000; va < 0x01000000; va += (1 << MB_SHIFT)) {
        unsigned int pa = va; // Assuming identity mapping for now
        unsigned int index = va >> MB_SHIFT;
        l1_start[index] = (l1_table_entry){
            .section = {.address = pa >> MB_SHIFT, .TEX = 0b001, .cacheable = true, .bufferable = true, .AP10 = 0b11, .AP2 = false}
        };
    }
    for (int i = 0; i < 2048; ++i) {
        const auto va = (0x40000000 >> 20) + i;
        l1_start[va] = (l1_table_entry){
            .section = {
                .address = va, .type = 0b10, .cacheable = true, .bufferable = true, .AP10 = 0b11,
                .AP2 = 0b0, .S = false, .nG = false, .XN = false, .TEX = 0b001
            }
        };
    }


    asm volatile("MCR p15, 0, %0, c7, c10, 0" :: "r" (0)); // Clean D-Cache to PoU (all entries)
    asm volatile("MCR p15, 0, %0, c7, c14, 0" :: "r" (0)); // Invalidate D-Cache (all entries)
    asm volatile("DSB" ::: "memory"); // Data Synchronization Barrier
    asm volatile("ISB" ::: "memory"); // Instruction Synchronization Barrier

    // Invalidate TLB (Translation Lookaside Buffer)
    asm volatile("MCR p15, 0, %0, c8, c7, 0" :: "r" (0)); // TLBIALL (Invalidate all TLB entries)
    asm volatile("DSB" ::: "memory"); // Data Synchronization Barrier
    asm volatile("ISB" ::: "memory"); // Instruction Synchronization Barrier

    // TTBR0
    asm volatile("MCR p15, 0, %0, c2, c0, 0" :: "r" (l1_start));

    // TTBCR
    asm volatile("MCR p15, 0, %0, c2, c0, 2" :: "r" (0)); // N = 0

    // enable mmu
    // 9. Enable MMU in SCTLR
    uint32_t sctlr_value;
    asm volatile("MRC p15, 0, %0, c1, c0, 0" : "=r" (sctlr_value)); // Read SCTLR
    sctlr_value |= 1 << 0; // Set M bit (MMU enable)
    sctlr_value |= 1 << 29;
    // Other bits you might want to set/clear in SCTLR:
    // (1 << 2) // C: Cache enable (Data cache)
    // (1 << 12) // I: Instruction cache enable
    // (1 << 13) // V: High vectors (0xFFFF0000 if not using low vectors)
    // (1 << 28) // TRE: TEX Remap Enable (if using TEX remapping)
    // (1 << 29) // AFE: Access Flag Enable (if using simplified access permissions without DACR domains)
    asm volatile("MCR p15, 0, %0, c1, c0, 0" :: "r" (sctlr_value)); // Write SCTLR
}
