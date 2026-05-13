//
// Created by Romir Kulshrestha on 12/05/2026.
//

#pragma once

#include "types.h"

// Physical addresses (used by tt.c to populate page table entries)
#define UART0_PHYSICAL 0x09000000u
#define GICD_PHYSICAL  0x08000000u
#define GICC_PHYSICAL  0x08010000u

// Each device gets its own 2MB virtual slot (one L2 block entry per physical 2MB block).
// Offsets must be multiples of 2MB (0x200000) since we use L2 block entries.
constexpr u64 DEVICE_VIRT_BLOCK = 0xFFFFFFFF'D0000000ULL;

constexpr u64 UART0_BASE       = DEVICE_VIRT_BLOCK + 0x0 * 0x200000; // L2[128] → PA 0x09000000
constexpr u64 GICD_BASE        = DEVICE_VIRT_BLOCK + 0x1 * 0x200000; // L2[129] → PA 0x08000000
constexpr u64 GICC_BASE        = GICD_BASE + 0x010000;               // PA 0x08010000, same L2 block
constexpr u64 RTC_BASE         = DEVICE_VIRT_BLOCK + 0x2 * 0x200000; // L2[130] — reserved
constexpr u64 FWCFG_BASE       = DEVICE_VIRT_BLOCK + 0x3 * 0x200000; // L2[131] — reserved
constexpr u64 VIRTIO_MMIO_BASE = DEVICE_VIRT_BLOCK + 0x4 * 0x200000; // L2[132] — reserved
