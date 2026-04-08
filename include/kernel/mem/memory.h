#pragma once
// memory.h — physical memory layout constants and address translation

#include "types.h"

#define KERNEL_VA    0xC0000000u // kernel virtual base address
#define KPHYS_OFFSET 0x100000u   // kernel physical base = DTB section base + this

// Physical address of the kernel image; set during early boot by kboot.
extern u32 kernel_phys_base;

// Convert a virtual address to physical.
// Kernel VA (>= KERNEL_VA) uses kernel_phys_base offset; otherwise identity-mapped.
[[gnu::const]]
static inline u32 virt_to_phys(const void *va) {
    u32 v = (u32) va;
    if (v >= KERNEL_VA) return kernel_phys_base + (v - KERNEL_VA);
    return v;
}