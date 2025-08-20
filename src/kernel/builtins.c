//
// Created by Romir Kulshrestha on 01/06/2025.
//

#include "builtins.h"
#include "types.h"

void *memcpy(void *dst, const void *src, size_t len) {
    u64 *dp64 = dst;
    const u64 *sp64 = src;
    while (len >= 8) {
        *dp64++ = *sp64++;
        len -= 8;
    }
    u8 *dp8 = (void *) dp64;
    const u8 *sp8 = (void *) sp64;
    while (len--)
        *dp8++ = *sp8++;

    return dst;
}

static void *rev_memcpy(void *dst, const void *src, size_t len) {
    u64 *dp64 = dst + len - 1;
    const u64 *sp64 = src + len - 1;
    while (len >= 8) {
        *dp64-- = *sp64--;
        len -= 8;
    }
    u8 *dp8 = (void *) dp64;
    const u8 *sp8 = (void *) sp64;
    while (len--)
        *dp8-- = *sp8--;

    return dst;
}

void *memmove(void *dst, const void *src, const size_t len) {
    if (dst < src)
        return memcpy(dst, src, len);
    if (dst > src)
        return rev_memcpy(dst, src, len);
    return dst;
}

void *memset(void *dst, const int c, size_t len) {
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

void *memclr(void *dst, const size_t len) {
    return memset(dst, 0, len);
}

// EABI functions

void __aeabi_memcpy(void *dst, const void *src, size_t n) {
    memcpy(dst, src, n);
}

void __aeabi_memcpy4(void *dst, const void *src, size_t n) {
    __aeabi_memcpy(dst, src, n);
}

void __aeabi_memset(void *dst, size_t n, int c) { // NOLINT(*-reserved-identifier)
    memset(dst, c, n);
}

void __aeabi_memset8(void *dst, size_t n, int c) { // NOLINT(*-reserved-identifier)
    __aeabi_memset(dst, n, c);
}

void __aeabi_memclr(void *dst, size_t n) { // NOLINT(*-reserved-identifier)
    memclr(dst, n);
}

void __aeabi_memclr8(void *dst, size_t n) {
    __aeabi_memclr(dst, n);
}

void __aeabi_memclr4(void *dst, size_t n) { // NOLINT(*-reserved-identifier)
    __aeabi_memclr(dst, n);
}