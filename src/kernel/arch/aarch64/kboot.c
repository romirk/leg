//
// Created by Romir Kulshrestha on 08/05/2026.
//

#include "types.h"

#define UARTDR (volatile u8 *) 0x09000000

// TTBR0 and TTBR1 input address size (granule size)
#define TCR_T0SZ(n)    ((u64) (n) << 0)
#define TCR_T1SZ(n)    ((u64) (n) << 16)
// Granule sizes: 4K for both TTBR0 and TTBR1
#define TCR_TG0_4K     (0b00ULL << 14)
#define TCR_TG1_4K     (0b10ULL << 30)
// Inner and outer cacheability: write-back, write-allocate (TTBR0)
#define TCR_IRGN0_WBWA (0b01ULL << 8)
#define TCR_ORGN0_WBWA (0b01ULL << 10)
// Shareability: inner shareable (TTBR0)
#define TCR_SH0_IS     (0b11ULL << 12)
// Inner and outer cacheability: write-back, write-allocate (TTBR1)
#define TCR_IRGN1_WBWA (0b01ULL << 24)
#define TCR_ORGN1_WBWA (0b01ULL << 26)
// Shareability: inner shareable (TTBR1)
#define TCR_SH1_IS     (0b11ULL << 28)
// Physical address size: 48 bits
#define TCR_IPS_48     (0b101ULL << 32)

// Index 0: 0xFF Normal cacheable (0xFF) — for kernel + user code/data
// Index 1: 0x00 Device-nGnRnE (0x00) — for UART, GIC, etc.
constexpr u64 MAIR_EL1 = 0x00000000000000FF;
constexpr u64 TCR_EL1 = TCR_T0SZ(25) | TCR_T1SZ(25) | TCR_TG0_4K | TCR_TG1_4K | TCR_IRGN0_WBWA |
                        TCR_ORGN0_WBWA | TCR_SH0_IS | TCR_IRGN1_WBWA | TCR_ORGN1_WBWA | TCR_SH1_IS |
                        TCR_IPS_48;

[[gnu::section(".boot.rodata")]]
constexpr char msg[] = "hello world!\n";

[[gnu::section(".boot")]]
void init_mmu() {
    asm volatile("msr mair_el1, %0" ::"r"(MAIR_EL1));
    asm volatile("msr tcr_el1, %0" ::"r"(TCR_EL1));
    asm volatile("isb");
}

[[gnu::section(".boot")]]
void kboot(uptr) {
    init_mmu();
    for (const char *c = msg; *c; c++) {
        *UARTDR = *c;
    }

    u64 mair, tcr;
    asm volatile("mrs %0, mair_el1" : "=r"(mair));
    asm volatile("mrs %0, tcr_el1" : "=r"(tcr));
}
