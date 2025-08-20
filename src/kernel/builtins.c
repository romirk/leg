//
// Created by Romir Kulshrestha on 01/06/2025.
//

#include "builtins.h"
#include "types.h"

void *memcpy(void *dst, const void *src, size_t len) {
    if (dst == src)
        return nullptr;
    u8 *dp8 = dst;
    const u8 *sp8 = (void *) src;
    while (len--)
        *dp8++ = *sp8++;

    return dst;
}

static void *revcpy(void *dst, const void *src, size_t len) {
    if (dst == src)
        return nullptr;
    u8 *dp8 = dst + len - 1;
    const u8 *sp8 = src + len - 1;
    while (len--)
        *dp8-- = *sp8--;

    return dst;
}

void *memmove(void *dst, const void *src, const size_t len) {
    if (dst < src)
        return memcpy(dst, src, len);
    if (dst > src)
        return revcpy(dst, src, len);
    return dst;
}

void *memset(void *dst, const int c, size_t len) {
    u8 *dp = dst;
    while (len--)
        *dp++ = c;
    return dst;
}

void *memclr(void *dst, const size_t len) {
    return memset(dst, 0, len);
}
