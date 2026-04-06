//
// Created by Romir Kulshrestha on 04/06/2025.
//

#ifndef UTILS_H
#define UTILS_H

#include "types.h"


#define loop  for(;;)
#define limbo loop asm("wfi")

#define get_bits(data, high, low)       (((u32)(data) & ~(0xffffffffu << ((high) + 1))) >> (low))
#define get_high_bits(data, n_bits)     ((u32)(data) >> (32 - (n_bits)))

inline void swap(char *a, char *b) {
    const auto t = *a;
    *a = *b;
    *b = t;
}

[[gnu::const]]
inline void *align(void *ptr, const u8 alignment) {
    return (void *) (-alignment & (u32) ptr + alignment - 1);
}

[[noreturn]]
void panic(char *msg);

// PSCI v0.2 via HVC (QEMU virt)
#define PSCI_SYSTEM_OFF  0x84000008u
#define PSCI_SYSTEM_RESET 0x84000009u

[[noreturn]]
inline void poweroff(void) {
    asm volatile("mov r0, %0\n\thvc #0" :: "r"(PSCI_SYSTEM_OFF) : "r0");
    __builtin_unreachable();
}

#endif //UTILS_H
