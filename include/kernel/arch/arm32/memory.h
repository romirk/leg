//
// Created by Romir Kulshrestha on 10/04/2026.
//

#ifndef LEG_MEMORY_H
#define LEG_MEMORY_H

// Kermel address
#define KERNEL_VA    0xC0000000u // kernel virtual base address
#define KPHYS_OFFSET 0x200000u   // kernel physical base = DTB section base + this

#define FWCFG_PHYSICAL 0x09020000u

// GIC base addresses (ARM virt machine)
#define GICD_PHYSICAL 0x08000000u
#define GICC_PHYSICAL 0x08010000u

#define RTC_PHYSICAL 0x09010000

#define UART0_PHYSICAL 0x09000000

// virtio-mmio bus: 32 slots at 0x0a000000, stride 0x200; SPI 16+N → GIC 48+N
#define VIRTIO_MMIO_PHYSICAL 0x0a000000u

#define DEVICE_VIRT_BLOCK 0xCF000000u

#define UART0_BASE       (DEVICE_VIRT_BLOCK + 0x00000) // 0xCF000000
#define GICD_BASE        (DEVICE_VIRT_BLOCK + 0x10000) // 0xCF010000
#define GICC_BASE        (DEVICE_VIRT_BLOCK + 0x20000) // 0xCF020000
#define RTC_BASE         (DEVICE_VIRT_BLOCK + 0x30000) // 0xCF030000
#define FWCFG_BASE       (DEVICE_VIRT_BLOCK + 0x40000) // 0xCF040000
#define VIRTIO_MMIO_BASE (DEVICE_VIRT_BLOCK + 0x50000) // 0xCF050000

#include "types.h"

// Physical address of the kernel image; set during early boot by kboot.
extern u32 kernel_phys_base;

// Convert a virtual address to physical.
// Kernel VA (>= KERNEL_VA) uses kernel_phys_base offset; otherwise identity-mapped.
[[gnu::pure]]
static inline u32 virt_to_phys(const void *va) {
    const u32 v = (u32) va;
    if (v >= KERNEL_VA) return kernel_phys_base + (v - KERNEL_VA);
    return v;
}

// Convert a kernel-heap physical address back to its virtual address.
[[gnu::pure]]
static inline void *phys_to_virt(u32 pa) {
    return (void *) (pa - kernel_phys_base + KERNEL_VA);
}

#endif // LEG_MEMORY_H
