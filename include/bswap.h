// bswap.h — byte-swap and big-endian read utilities

#ifndef LEG_BSWAP_H
#define LEG_BSWAP_H

#include "types.h"

// in-register byte swap (compiler emits `rev`/`rev16`)

[[gnu::const]]
inline u16 bswap16(const u16 x) {
    const u16 lo = x >> 8 & 0xFF;
    const u16 hi = (x & 0xFF) << 8;
    return hi | lo;
}

[[gnu::const]]
inline u32 bswap32(u32 x) {
    const u32 b0 = (x & 0x000000FFu) << 24u;
    const u32 b1 = (x & 0x0000FF00u) << 8u;
    const u32 b2 = (x & 0x00FF0000u) >> 8u;
    const u32 b3 = (x & 0xFF000000u) >> 24u;
    return b0 | b1 | b2 | b3;
}

[[gnu::const]]
inline u64 bswap64(u64 x) {
    const u64 hi = (u64) bswap32((u32) x) << 32;
    const u64 lo = bswap32((u32) (x >> 32));
    return hi | lo;
}

// big-endian reads from memory (safe on any alignment for ARMv7)

static u32 be32_read(const void *p) {
    const u8 *b = p;
    return (u32) b[0] << 24 | (u32) b[1] << 16 | (u32) b[2] << 8 | (u32) b[3] << 0;
}

static u64 be64_read(const void *p) {
    const u64 hi = (u64) be32_read(p) << 32;
    const u64 lo = be32_read((const u8 *) p + 4);
    return hi | lo;
}

#endif // LEG_BSWAP_H
