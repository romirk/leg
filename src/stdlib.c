//
// Created by Romir Kulshrestha on 02/06/2025.
//

#include "stdlib.h"

#include "linker.h"
#include "uart.h"

char *itoa(i32 value, char *str) {
    if (value == 0) {
        str[0] = '0';
        str[1] = '\0';
        return str;
    }

    auto p = str;
    if (value < 0) {
        *p++ = '-';
        value = -value;
    }

    u8 digits[19]; // INT64_MAX has 19 digits
    auto i = 0u;

    do {
        const u8 digit = value % 10;
        digits[i++] = digit + '0';
    } while (value /= 10);

    while (i > 0) {
        *p++ = digits[--i];
    }
    *p = '\0';
    return str;
}

char *hex(i64 value, char *str) {
    if (value == 0) {
        str[0] = '0';
        str[1] = 'x';
        str[2] = '0';
        str[3] = '\0';
        return str;
    }

    auto p = str;
    if (value < 0) {
        *p++ = '-';
        value = -value;
    }

    *p++ = '0';
    *p++ = 'x';

    u8 digits[16];
    auto i = 0u;

    do {
        const auto hex_digits = "0123456789abcdef";
        const u8 digit = value % 16;
        digits[i++] = hex_digits[digit];
    } while (value /= 16);

    while (i > 0) {
        *p++ = digits[--i];
    }
    *p = '\0';
    return str;
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
    print("PANIC!: ");
    println(msg);

    print("debug info:\nstack: *");
    void *p = nullptr;
    void *sp = &p;
    const auto stack_height = STACK_BOTTOM - sp;
    print_ptr(sp);
    putchar('\n');

    hexdump(sp, stack_height > 64 ? 64 : stack_height);

    for (;;) {
        asm ("wfi");
    }
}
