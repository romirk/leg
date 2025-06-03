//
// Created by Romir Kulshrestha on 02/06/2025.
//

#ifndef STDLIB_H
#define STDLIB_H
#include "types.h"

char *itoa(i32 value, char *p);

char *hex(i64 value, char *p);

char *hex8(u8 value, char *str);

char *hex32le(u32 value, char *str);
char *hex32be(u32 value, char *str);

[[gnu::const]]
inline u32 swap_endianness(u32 num) {
    // Compiler will optimize this to the single instruction `rev`
    u32 b0, b1, b2, b3;
    b0 = (num & 0x000000ff) << 24u;
    b1 = (num & 0x0000ff00) << 8u;
    b2 = (num & 0x00ff0000) >> 8u;
    b3 = (num & 0xff000000) >> 24u;
    return b0 | b1 | b2 | b3;
}

void panic(char *msg);

#endif //STDLIB_H
