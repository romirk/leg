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

static int print_i32(const i32 num) {
    char res[12];
    itoa(num, res, 10);
    return print(res);
}

static int print_base(const u32 num, const u8 base) {
    char res[34];
    utoa(num, res, base);
    return print(res);
}

// there's a subtle difference between this and `print_hex`: this one always prints all 4 bytes.
static int print_ptr(void *ptr) {
    char res[9];
    hex32be((u32) ptr, res);
    return print(res);
}

[[gnu::const]]
static bool is_printable(const char c) {
    return c >= 32 && c <= 126;
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
                n += print_base(va_arg(args, u32), 10);
                break;
            case 'x':
                n += print_base(va_arg(args, u32), 16);
                break;
            case 'o':
                n += print_base(va_arg(args, u32), 8);
                break;
            case 'b':
                n += print_base(va_arg(args, u32), 2);
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

void hexdump(const void *ptr, const u32 len) {
    u32 *addr = (u32 *) ptr;
    for (u32 i = 1; i <= len; i++) {
        if (i % 4 == 1) {
            print_ptr((void *) addr);
            putchar('\t');
        }
        char res[9];
        hex32le(*addr++, res);
        print(res);

        if (i % 4)
            putchar(' ');
        else {
            print("\t│ ");

            auto start = (u8 *) (addr - 4);
            for (int j = 0; j < 16; j++) {
                if (!is_printable(start[j]))
                    putchar('.');
                else
                    putchar(start[j]);
            }

            print(" │\n");
        }
    }
    putchar('\n');
}
