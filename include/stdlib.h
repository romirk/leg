//
// Created by Romir Kulshrestha on 02/06/2025.
//

#ifndef STDLIB_H
#define STDLIB_H
#include "types.h"

extern const char * const ALPHANUM;

/**
 * Convert <code>unsigned int</code> into a string
 * @param value value to convert
 * @param buffer output buffer
 * @param radix Numerical base used to represent the value as a string, between \c 2 and \c 36
 * @return
 */
char *utoa(u32 value, char *buffer, u8 radix);

/**
 * Convert \c int into a string
 * @param value value to convert
 * @param buffer output buffer
 * @param radix Numerical base used to represent the value as a string, between \c 2 and \c 36
 * @return
 */
char *itoa(i32 value, char *buffer, u8 radix);

/**
 * Converts a byte into a 2-digit hex, represented as a \c u16
 * @param digit byte
 * @return hex representation of byte as a \c u16
 */
u16 hex8(u8 digit);

char *hex32le(u32 value, char str[9]);

char *hex32be(u32 value, char str[9]);

[[gnu::const]]
inline u32 swap_endianness(const u32 num) {
    // Compiler will optimize this to the single instruction `rev`
    const u32 b0 = (num & 0x000000ff) << 24u;
    const u32 b1 = (num & 0x0000ff00) << 8u;
    const u32 b2 = (num & 0x00ff0000) >> 8u;
    const u32 b3 = (num & 0xff000000) >> 24u;
    return b0 | b1 | b2 | b3;
}

void panic(char *msg);

#endif //STDLIB_H
