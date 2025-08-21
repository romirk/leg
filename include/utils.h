//
// Created by Romir Kulshrestha on 04/06/2025.
//

#ifndef UTILS_H
#define UTILS_H

#include "types.h"

#define limbo for(;;) asm("wfi")

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
#endif //UTILS_H
