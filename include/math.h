//
// Created by Romir Kulshrestha on 04/06/2025.
//

#ifndef MATH_H
#define MATH_H
#include "types.h"

#define log2(X) ((unsigned) (8 * sizeof(unsigned long long) - __builtin_clzll((X)) - 1))

[[gnu::const]]
inline u32 log(const u32 value, const u8 base) {
    return log2(value) / log2(base);
}

#endif //MATH_H
