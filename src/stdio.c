//
// Created by Romir Kulshrestha on 03/06/2025.
//

#include "stdio.h"

#include <stdarg.h>

#include "stdlib.h"
#include "uart.h"

void print(const char *s) {
    while (*s)
        putchar(*s++);
}

void println(const char *s) {
    print(s);
    putchar('\n');
}

void print_u32(const u32 num) {
    char res[12];
    utoa(num, res, 10);
    print(res);
}

void print_i32(const i32 num) {
    char res[12];
    itoa(num, res, 10);
    print(res);
}

void print_hex(const u32 num) {
    char res[9];
    utoa(num, res, 16);
    print(res);
}

// there's a subtle difference between this and `print_hex`: this one always prints all 4 bytes.
void print_ptr(void *ptr) {
    char res[9];
    hex32be((u32) ptr, res);
    print(res);
}

void print_bool(const bool value) {
    print(value ? "true" : "false");
}

void printf(const char *fmt, ...) {
    va_list args;
    va_start(args);

    char c;
    while ((c = *fmt++)) {
        if (c != '%') {
            putchar(c);
            continue;
        }
        const char t = *fmt++;
        switch (t) {
            case 's':
                print(va_arg(args, char *));
                break;
            case 'c':
                putchar(va_arg(args, int));
                break;
            case 'd':
                print_i32(va_arg(args, i32));
                break;
            case 'u':
                print_u32(va_arg(args, u32));
                break;
            case 'x':
                print_hex(va_arg(args, u32));
                break;
            case 'p':
                print_ptr(va_arg(args, void *));
                break;
            default:
                printf("unknown format: %c\n", c);
        }
    }
    va_end(args);
}

void hexdump(u32 *addr, u32 len) {
    for (u32 i = 1; i <= len; i++) {
        char res[9];
        hex32le(*addr++, res);
        print(res);
        putchar(i % 4 ? ' ' : '\n');
    }
    putchar('\n');
}
