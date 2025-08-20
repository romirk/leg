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
