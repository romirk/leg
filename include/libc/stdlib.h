#ifndef STDLIB_H
#define STDLIB_H
#include "types.h"

extern const char *const ALPHANUM;

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

double atof(const char *s);

#endif // STDLIB_H
