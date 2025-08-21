//
// Created by Romir Kulshrestha on 03/06/2025.
//

#include "stdio.h"

#include <stdarg.h>

#include "builtins.h"
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

void pprintf(const char *fmt, ...) {
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
                n += print("\033[97m");
                n += print_i32(va_arg(args, i32));
                n += print("\033[0m");
                break;
            case 'u':
                n += print("\033[97m");
                n += print_base(va_arg(args, u32), 10);
                n += print("\033[0m");
                break;
            case 'x':
                n += print("\033[97m");
                n += print_base(va_arg(args, u32), 16);
                n += print("\033[0m");
                break;
            case 'o':
                n += print("\033[97m");
                n += print_base(va_arg(args, u32), 8);
                n += print("\033[0m");
                break;
            case 'b':
                n += print("\033[97m");
                n += print_base(va_arg(args, u32), 2);
                n += print("\033[0m");
                break;
            case 't':
                n += print("\033[97m");
                n += print(va_arg(args, int) ? "true" : "false");
                n += print("\033[0m");
                break;
            case 'p':
                n += print("\033[31;49m");
                n += print_ptr(va_arg(args, void *));
                n += print("\033[0m");
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

[[maybe_unused]]
static void hexdump_ansi(const void *ptr, const u32 len) {
    auto addr = (u32 *) ptr;
    for (u32 i = 0; i < len; i++) {
        if (i % 4 == 0) {
            print("\n\033[2;37m");
            print_ptr(addr);
            print("\033[36C\t|\033[18C|\033[0m");
        }

        const auto data = *addr++;

        printf("\033[%uG", 12 + 9 * (i % 4));

        char res[9];
        hex32le(data, res);
        print(res);

        printf("\033[%uG", 51 + 4 * (i % 4));

        const u8 b1 = (data & 0x000000FF) >> 0;
        const u8 b2 = (data & 0x0000FF00) >> 8;
        const u8 b3 = (data & 0x00FF0000) >> 16;
        const u8 b4 = (data & 0xFF000000) >> 24;

        putchar(is_printable(b1) ? b1 : '.');
        putchar(is_printable(b2) ? b2 : '.');
        putchar(is_printable(b3) ? b3 : '.');
        putchar(is_printable(b4) ? b4 : '.');
    }
    putchar('\n');
}

[[maybe_unused]]
static void hexdump_buffer(const void *ptr, const u32 len) {
    auto addr = (u32 *) ptr;

    char buffer[8 + 4 + (8 + 1) * 4 + 3 + 16 + 3 + 1];
    memset(buffer, ' ', sizeof(buffer));
    buffer[sizeof(buffer) - 1] = '\0';
    buffer[sizeof(buffer) - 2] = '|';
    buffer[sizeof(buffer) - 21] = '|';

    for (u32 i = 0; i < len; i++) {
        if (i % 4 == 0) {
            // print("\n\033[2;37m");
            if (i) puts(buffer);
            hex32be((u32) addr, buffer);
            buffer[8] = ' ';
            // print("\033[36C\t|\033[18C|\033[0m");
        }

        const auto data = *addr++;

        // printf("\033[%uG", );
        hex32le(data, buffer + 12 + 9 * (i % 4));
        buffer[20 + 9 * (i % 4)] = ' ';
        // print(res);

        // printf("\033[%uG", 51 + 4 * (i % 4));
        const auto start = 52 + 4 * (i % 4);

        const u8 b1 = (data & 0x000000FF) >> 0u;
        const u8 b2 = (data & 0x0000FF00) >> 8u;
        const u8 b3 = (data & 0x00FF0000) >> 16;
        const u8 b4 = (data & 0xFF000000) >> 24;


        buffer[start] = is_printable(b1) ? b1 : '.';
        buffer[start + 1] = is_printable(b2) ? b2 : '.';
        buffer[start + 2] = is_printable(b3) ? b3 : '.';
        buffer[start + 3] = is_printable(b4) ? b4 : '.';
    }
    putchar('\n');
}

void hexdump(const void *ptr, const u32 len) {
#ifdef NO_ANSI
    hexdump_buffer(ptr, len);
#else
    hexdump_ansi(ptr, len);
#endif
}
