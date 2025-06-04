//
// Created by Romir Kulshrestha on 02/06/2025.
//

#include "stdlib.h"

#include "linker.h"
#include "logs.h"
#include "stdio.h"
#include "uart.h"


char *utoa(u32 value, char *str, u8 base) {
    if (value == 0) {
        str[2] = '0';
        str[3] = '\0';
        return str;
    }

    auto p = str;
    u8 digits[16];
    auto i = 0u;

    do {
        const auto hex_digits = "0123456789abcdef";
        const u8 digit = value % base;
        digits[i++] = hex_digits[digit];
    } while (value /= base);

    while (i > 0) {
        *p++ = digits[--i];
    }
    *p = '\0';
    return str;
}

char *itoa(i32 value, char *p, u8 base) {
    if (value < 0) {
        *p++ = '-';
        value = -value;
    }
    utoa(value, p, base);
    return p;
}

char *hex8(u8 value, char *str) {
    const auto hex_digits = "0123456789abcdef";
    str[1] = hex_digits[value % 16];
    str[0] = hex_digits[value / 16];
    return str;
}

char *hex32le(u32 value, char *str) {
    hex8(value & 0xff, str);
    hex8(value >> 8 & 0xff, str + 2);
    hex8(value >> 16 & 0xff, str + 4);
    hex8(value >> 24 & 0xff, str + 6);
    str[8] = '\0';
    return str;
}

char *hex32be(u32 value, char *str) {
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
