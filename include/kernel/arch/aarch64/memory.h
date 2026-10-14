//
// Created by Romir Kulshrestha on 12/05/2026.
//

#pragma once

#include "types.h"

constexpr uptr  MB = 0x00100000; // one megabyte
constexpr u8 AS_MB = 20;

constexpr uptr RAM_BASE       = 0x00000000'40000000;
constexpr uptr KERNEL_PA      = 0x00000000'40200000;
constexpr uptr PAGE_OFFSET    = 0xFFFFFF80'00000000;

// Physical addresses (used by tt.c to populate page table entries)
constexpr uptr UART0_PHYSICAL = 0x09000000u;
constexpr uptr GICD_PHYSICAL  = 0x08000000u;
constexpr uptr GICC_PHYSICAL  = 0x08010000u;

constexpr uptr UART0_BASE       = UART0_PHYSICAL + PAGE_OFFSET;
constexpr uptr GICD_BASE        = GICD_PHYSICAL + PAGE_OFFSET;
constexpr uptr GICC_BASE        = GICC_PHYSICAL + PAGE_OFFSET;

// constexpr uptr RTC_BASE         = PAGE_OFFSET + 0x2 * 0x200000; // L2[130] — reserved
// constexpr uptr FWCFG_BASE       = PAGE_OFFSET + 0x3 * 0x200000; // L2[131] — reserved
// constexpr uptr VIRTIO_MMIO_BASE = PAGE_OFFSET + 0x4 * 0x200000; // L2[132] — reserved
