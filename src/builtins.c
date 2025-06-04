//
// Created by Romir Kulshrestha on 01/06/2025.
//

#include "builtins.h"
#include "types.h"

[[gnu::used]]
void *memcpy(void *dst, const void *src, size_t len) {
    if (len < 4) goto c1;
    u32 *dp32 = dst;
    const u32 *sp32 = src;
    while (len >= 4) {
        *dp32++ = *sp32++;
        len -= 4;
    }
    dst = dp32;
    src = sp32;
c1:
    u8 *dp = dst;
    const u8 *sp = src;
    while (len--)
        *dp++ = *sp++;
    return dst;
}

[[gnu::used]]
void *memmove(void *dst, const void *src, size_t len) {
    if (dst < src)
        return memcpy(dst, src, len);
    if (dst > src) {
        u8 *dp = dst + len - 1;
        const u8 *sp = src + len - 1;
        while (len--)
            *dp-- = *sp--;
    }
    return dst;
}

[[gnu::used]]
void *memset(void *dst, int c, size_t len) {
    u8 *dp = dst;
    while ((u32) dp % 4 && len--)
        *dp++ = c;
    u64 l = c;
    l |= l << 8;
    l |= l << 16;
    l |= l << 32;
    while (len >= 8) {
        *(u64 *) dp = l;
        dp += 8;
        len -= 8;
    }
    while (len--)
        *dp++ = c;
    return dst;
}

[[gnu::used]]
void *memclr(void *dst, size_t len) {
    return memset(dst, 0, len);
}

// EABI functions
//
// [[gnu::used]]
// void __aeabi_memcpy(void *dst, const void *src, size_t n) {
//     memcpy(dst, src, n);
// }
//
// [[gnu::used]]
// void __aeabi_memcpy4(void *dst, const void *src, size_t n) {
//     __aeabi_memcpy(dst, src, n);
// }
//
// [[gnu::used]]
// void __aeabi_memset(void *dst, size_t n, int c) { // NOLINT(*-reserved-identifier)
//     memset(dst, c, n);
// }
//
// [[gnu::used]]
// void __aeabi_memset8(void *dst, size_t n, int c) { // NOLINT(*-reserved-identifier)
//     __aeabi_memset(dst, n, c);
// }
//
// [[gnu::used]]
// void __aeabi_memclr(void *dst, size_t n) { // NOLINT(*-reserved-identifier)
//     memclr(dst, n);
// }
//
// [[gnu::used]]
// void __aeabi_memclr8(void *dst, size_t n) {
//     __aeabi_memclr(dst, n);
// }
//
// [[gnu::used]]
// void __aeabi_memclr4(void *dst, size_t n) { // NOLINT(*-reserved-identifier)
//     __aeabi_memclr(dst, n);
// }
