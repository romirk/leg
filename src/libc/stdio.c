#include "libc/stdio.h"

#include "libc/builtins.h"
#include "libc/stdlib.h"
#include "syscall.h"

#include <stdarg.h>

void putchar(char c) {
    sys_write(&c, 1);
}

char getchar(void) {
    return sys_getchar();
}

char getchar_nb(void) {
    return sys_getchar_nb();
}

u32 readline(char *buf, u32 max) {
    return sys_read(buf, max);
}

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

void printf(const char *fmt, ...) {
    va_list args;
    va_start(args);

    char c;
    int  n = 0;
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
                *ptr           = n;
                break;
            default:
                print("?%");
                putchar(t);
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
    int  n = 0;
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
                *ptr           = n;
                break;
            default:
                print("?%");
                putchar(t);
                va_end(args);
                return;
        }
    }
    va_end(args);
}
