//
// Created by Romir Kulshrestha on 04/06/2025.
//

#ifndef UTILS_H
#define UTILS_H

#include "types.h"

#define limbo for(;;) asm("wfi")

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
