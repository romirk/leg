//
// rng.c — xorshift32 PRNG + urandom
//

#include "kernel/rng.h"

static u32 state = 0xDEADBEEF;

void rng_seed(u32 seed) {
    state = seed ? seed : 0xDEADBEEF;
}

u32 rand32(void) {
    u32 x = state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    state = x;
    return x;
}

u32 rand_below(u32 bound) {
    return rand32() % bound;
}

void urandom(void *buf, u32 n) {
    u8 *p = buf;
    while (n >= 4) {
        u32 r = rand32();
        p[0] = (u8) r;
        p[1] = (u8)(r >> 8);
        p[2] = (u8)(r >> 16);
        p[3] = (u8)(r >> 24);
        p += 4;
        n -= 4;
    }
    if (n) {
        u32 r = rand32();
        for (u32 i = 0; i < n; i++) {
            p[i] = (u8) r;
            r >>= 8;
        }
    }
}