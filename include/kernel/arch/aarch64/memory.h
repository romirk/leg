//
// Created by Romir Kulshrestha on 12/05/2026.
//

#pragma once

#include "types.h"

constexpr uptr KERNEL_PA        = 0x00000000'40100000;
constexpr uptr KERNEL_VA        = 0xFFFFFFFF'C0100000;
constexpr uptr KERNEL_VA_OFFSET = KERNEL_VA - KERNEL_PA;

// Physical addresses (used by tt.c to populate page table entries)
constexpr uptr UART0_PHYSICAL = 0x09000000u;
constexpr uptr GICD_PHYSICAL  = 0x08000000u;
constexpr uptr GICC_PHYSICAL  = 0x08010000u;

// Each device gets its own 2MB virtual slot (one L2 block entry per physical 2MB block).
// Offsets must be multiples of 2MB (0x200000) since we use L2 block entries.
constexpr uptr DEVICE_VIRT_BLOCK = 0xFFFFFFFF'D0000000ULL;

constexpr uptr UART0_BASE       = DEVICE_VIRT_BLOCK + 0x0 * 0x200000; // L2[128] → PA 0x09000000
constexpr uptr GICD_BASE        = DEVICE_VIRT_BLOCK + 0x1 * 0x200000; // L2[129] → PA 0x08000000
constexpr uptr GICC_BASE        = GICD_BASE + 0x010000; // PA 0x08010000, same L2 block
constexpr uptr RTC_BASE         = DEVICE_VIRT_BLOCK + 0x2 * 0x200000; // L2[130] — reserved
constexpr uptr FWCFG_BASE       = DEVICE_VIRT_BLOCK + 0x3 * 0x200000; // L2[131] — reserved
constexpr uptr VIRTIO_MMIO_BASE = DEVICE_VIRT_BLOCK + 0x4 * 0x200000; // L2[132] — reserved
