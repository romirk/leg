//
// Created by Romir Kulshrestha on 01/06/2025.
//

#include "builtins.h"
#include "types.h"

[[gnu::used, clang::no_builtin]]
void *memcpy(void *dst, const void *src, size_t len) {
    u8 *dp = dst;
    const u8 *sp = src;
    while (len--) *dp++ = *sp++;
    return dst;
}

[[gnu::used, clang::no_builtin]]
void *memset(void *dst, int c, size_t len) {
    u8 *dp = dst;
    while ((u32) dp % 4 && len--) *dp++ = c;
    u64 l = c;
    l |= l << 8;
    l |= l << 16;
    l |= l << 32;
    while (len >= 8) {
        *(u64 *) dp = l;
        dp += 8;
        len -= 8;
    }
    while (len--) *dp++ = c;
    return dst;
}

[[gnu::used, clang::no_builtin]]
void *memclr(void *dst, size_t len) {
    return memset(dst, 0, len);
}

// EABI functions

[[gnu::used, clang::no_builtin]]
void __aeabi_memcpy(void *dst, const void *src, size_t n) {
    memcpy(dst, src, n);
}

[[gnu::used, clang::no_builtin]]
void __aeabi_memcpy4(void *dst, const void *src, size_t n) {
    __aeabi_memcpy(dst, src, n);
}

[[gnu::used, clang::no_builtin]]
void __aeabi_memset(void *dst, size_t n, int c) {
    memset(dst, c, n);
}

[[gnu::used, clang::no_builtin]]
void __aeabi_memset8(void *dst, size_t n, int c) {
    __aeabi_memset(dst, n, c);
}

[[gnu::used, clang::no_builtin]]
void __aeabi_memclr(void *dst, size_t n) {
    memclr(dst, n);
}

[[gnu::used, clang::no_builtin]]
void __aeabi_memclr8(void *dst, size_t n) {
    __aeabi_memclr(dst, n);
}

[[gnu::used, clang::no_builtin]]
void __aeabi_memclr4(void *dst, size_t n) {
    __aeabi_memclr(dst, n);
}
