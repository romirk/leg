//
// Created by Romir Kulshrestha on 03/06/2025.
//

#include "stdio.h"

#include <stdarg.h>

#include "logs.h"
#include "stdlib.h"
#include "uart.h"

int print(const char *p) {
    auto s = p;
    while (*s)
        putchar(*s++);
    return s - p;
}

int puts(const char *s) {
    const auto n = print(s);
    putchar('\n');
    return n + 1;
}

static int print_u32(const u32 num) {
    char res[12];
    utoa(num, res, 10);
    return print(res);
}

static int print_i32(const i32 num) {
    char res[12];
    itoa(num, res, 10);
    return print(res);
}

static int print_hex(const u32 num) {
    char res[9];
    utoa(num, res, 16);
    return print(res);
}

static int print_bin(const u32 num) {
    char res[32];
    utoa(num, res, 2);
    return print(res);
}

static int print_oct(const u32 num) {
    char res[12];
    utoa(num, res, 8);
    return print(res);
}

// there's a subtle difference between this and `print_hex`: this one always prints all 4 bytes.
static int print_ptr(void *ptr) {
    char res[9];
    hex32be((u32) ptr, res);
    return print(res);
}

void printf(const char *fmt, ...) {
    va_list args;
    va_start(args);

    char c;
    int n = 0;
    while ((c = *fmt++)) {
        if (c != '%') {
            putchar(c);
            n++;
            continue;
        }
        const char t = *fmt++;
        switch (t) {
            case '%':
                putchar('%');
                n++;
                break;
            case 's':
                n += print(va_arg(args, char *));
                break;
            case 'c':
                putchar(va_arg(args, int));
                n++;
                break;
            case 'd':
            case 'i':
                n += print_i32(va_arg(args, i32));
                break;
            case 'u':
                n += print_u32(va_arg(args, u32));
                break;
            case 'x':
                n += print_hex(va_arg(args, u32));
                break;
            case 'o':
                n += print_oct(va_arg(args, u32));
                break;
            case 'b':
                n += print_bin(va_arg(args, u32));
                break;
            case 't':
                n += print(va_arg(args, int) ? "true" : "false");
                break;
            case 'p':
                n += print_ptr(va_arg(args, void *));
                break;
            case 'n':
                const auto ptr = va_arg(args, signed int *);
                *ptr = n;
                break;
            default:
                putchar('\n');
                err("printf: unknown format %%%c", t);
                va_end(args);
                return;
        }
    }
    va_end(args);
}

void hexdump(const u32 *addr, const u32 len) {
    for (u32 i = 1; i <= len; i++) {
        char res[9];
        hex32le(*addr++, res);
        print(res);
        putchar(i % 4 ? ' ' : '\n');
    }
    putchar('\n');
}