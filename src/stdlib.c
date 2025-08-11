//
// Created by Romir Kulshrestha on 02/06/2025.
//

#include "stdlib.h"

#include "linker.h"
#include "logs.h"
#include "math.h"
#include "stdio.h"

const char * const ALPHANUM = "0123456789abcdefghijklmnopqrstuvwxyz";

char *utoa(u32 value, char *str, const u8 base) {
    if (value == 0) {
        str[0] = '0';
        str[1] = '\0';
        return str;
    }

    const auto digit_count = log(value, base);
    auto i = 0u;
    do {
        const u8 digit = value % base;
        str[digit_count - i++] = ALPHANUM[digit];
    } while (value /= base);

    str[i] = '\0';
    return str;
}

char *itoa(i32 value, char *str, const u8 base) {
    if (value < 0) {
        *str++ = '-';
        value = -value;
    }
    utoa(value, str, base);
    return str;
}

char *hex8(const u8 value, char *str) {
    str[1] = ALPHANUM[value % 16];
    str[0] = ALPHANUM[value / 16];
    return str;
}

char *hex32le(const u32 value, char str[9]) {
    hex8(value & 0xff, str);
    hex8(value >> 8 & 0xff, str + 2);
    hex8(value >> 16 & 0xff, str + 4);
    hex8(value >> 24 & 0xff, str + 6);
    str[8] = '\0';
    return str;
}

char *hex32be(const u32 value, char str[9]) {
    hex8(value & 0xff, str + 6);
    hex8(value >> 8 & 0xff, str + 4);
    hex8(value >> 16 & 0xff, str + 2);
    hex8(value >> 24 & 0xff, str);
    str[8] = '\0';
    return str;
}

[[noreturn]]
void panic(char *msg) {
    err("PANIC: %s", msg);

    print("debug info:\nstack: *");
    void *p = nullptr;
    void *sp = &p;
    const auto stack_height = (void *) STACK_BOTTOM - sp;

    printf("%p\n", sp);
    hexdump(sp, stack_height > 64 ? 64 : stack_height);

    for (;;) {
        asm ("wfi");
    }
}