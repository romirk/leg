//
// Created by Romir Kulshrestha on 01/06/2025.
//

#ifndef STDLIB_H
#define STDLIB_H
#include <stddef.h>

/**
 * Copy len bytes from src to dst.
 */
void *memcpy(void *dst, const void *src, size_t len);

/**
 * Set len bytes in dst to c.
 */
void *memset(void *dst, int c, size_t len);

/**
 * Set len bytes in dst to 0.
 */
void *memclr(void *dst, size_t len);

// EABI functions

void __aeabi_memcpy(void *dst, const void *src, size_t n);

void __aeabi_memcpy4(void *dest, const void *src, size_t n);

void __aeabi_memclr8(void *dst, size_t n);


#endif // STDLIB_H
