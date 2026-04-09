#ifndef MATH_H
#define MATH_H
#include "types.h"

#define log2(X) ((unsigned) (8 * sizeof(u64) - __builtin_clzll(X) - 1))

[[gnu::const, maybe_unused]]
static u32 log(u32 value, const u8 base) {
    u32 n = 0;
    while (value >= (u32) base) {
        value /= base;
        n++;
    }
    return n;
}

#endif // MATH_H
