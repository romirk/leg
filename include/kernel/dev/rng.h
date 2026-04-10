// rng.h — pseudo-random number generation

#ifndef LEG_RNG_H
#define LEG_RNG_H

#include "types.h"

// seed the PRNG (call once, e.g. from rtc_ticks())
void sys_rng_seed(u32 seed);

// return a pseudo-random 32-bit value
u32 sys_rand32(void);

// return a pseudo-random value in [0, bound)
u32 sys_rand_below(u32 bound);

// fill buf with n pseudo-random bytes
void urandom(void *buf, u32 n);

#endif // LEG_RNG_H
