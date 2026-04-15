// logs.c — kernel-only kprint functions that write directly to UART.
// Used by the log macros in logs.h; bypasses tty/fb entirely.

#include "kernel/logs.h"
#include "kernel/dev/uart.h"
#include "libc/builtins.h"
#include "libc/stdlib.h"

#include <stdarg.h>

static void kputchar(const char c) {
    uart_putchar(c);
}

int kprint(const char *s) {
    const char *p = s;
    while (*p)
        kputchar(*p++);
    return p - s;
}

static void kprint_i32(const i32 num) {
    char buf[12];
    itoa(num, buf, 10);
    kprint(buf);
}

static void kprint_base(const u32 num, const u8 base) {
    char buf[34];
    utoa(num, buf, base);
    kprint(buf);
}

static void kprint_ptr(const void *ptr) {
    char buf[9];
    hex32be((u32) ptr, buf);
    kprint(buf);
}

void vkprintf(const char *fmt, va_list args) {
    char c;
    while ((c = *fmt++)) {
        if (c != '%') {
            kputchar(c);
            continue;
        }
        switch (*fmt++) {
            case '%':
                kputchar('%');
                break;
            case 's':
                kprint(va_arg(args, char *));
                break;
            case 'c':
                kputchar((char) va_arg(args, int));
                break;
            case 'd':
            case 'i':
                kprint_i32(va_arg(args, i32));
                break;
            case 'u':
                kprint_base(va_arg(args, u32), 10);
                break;
            case 'x':
                kprint_base(va_arg(args, u32), 16);
                break;
            case 'o':
                kprint_base(va_arg(args, u32), 8);
                break;
            case 'b':
                kprint_base(va_arg(args, u32), 2);
                break;
            case 't':
                kprint(va_arg(args, int) ? "true" : "false");
                break;
            case 'p':
                kprint_ptr(va_arg(args, void *));
                break;
            default:
                kprint("?%");
                kputchar(*(fmt - 1));
                break;
        }
    }
}

void kprintf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vkprintf(fmt, args);
    va_end(args);
}

[[gnu::const]]
static bool is_printable(const char c) {
    return c >= 32 && c <= 126;
}

[[maybe_unused]]
static void hexdump_ansi(const void *ptr, const u32 len) {
    auto addr = (u32 *) ptr;
    for (u32 i = 0; i < len; i++) {
        if (i % 4 == 0) {
            kprint("\n\033[2;37m");
            kprint_ptr(addr);
            kprint("\033[36C\t|\033[18C|\033[0m");
        }

        const auto data = *addr++;

        kprintf("\033[%uG", 12 + 9 * (i % 4));

        char res[9];
        hex32le(data, res);
        kprint(res);

        kprintf("\033[%uG", 51 + 4 * (i % 4));

        const u8 b1 = (data & 0x000000FF) >> 0;
        const u8 b2 = (data & 0x0000FF00) >> 8;
        const u8 b3 = (data & 0x00FF0000) >> 16;
        const u8 b4 = (data & 0xFF000000) >> 24;

        kputchar(is_printable(b1) ? b1 : '.');
        kputchar(is_printable(b2) ? b2 : '.');
        kputchar(is_printable(b3) ? b3 : '.');
        kputchar(is_printable(b4) ? b4 : '.');
    }
    kputchar('\n');
}

[[maybe_unused]]
static void hexdump_buffer(const void *ptr, const u32 len) {
    auto addr = (u32 *) ptr;

    char buffer[8 + 4 + (8 + 1) * 4 + 3 + 16 + 3 + 1];
    memset(buffer, ' ', sizeof(buffer));
    buffer[sizeof(buffer) - 1]  = '\0';
    buffer[sizeof(buffer) - 2]  = '|';
    buffer[sizeof(buffer) - 21] = '|';

    for (u32 i = 0; i < len; i++) {
        if (i % 4 == 0) {
            // kprint("\n\033[2;37m");
            if (i) uart_puts(buffer);
            hex32be((u32) addr, buffer);
            buffer[8] = ' ';
            // kprint("\033[36C\t|\033[18C|\033[0m");
        }

        const auto data = *addr++;

        // kprintf("\033[%uG", );
        hex32le(data, buffer + 12 + 9 * (i % 4));
        buffer[20 + 9 * (i % 4)] = ' ';
        // kprint(res);

        // kprintf("\033[%uG", 51 + 4 * (i % 4));
        const auto start = 52 + 4 * (i % 4);

        const u8 b1 = (data & 0x000000FF) >> 0u;
        const u8 b2 = (data & 0x0000FF00) >> 8u;
        const u8 b3 = (data & 0x00FF0000) >> 16;
        const u8 b4 = (data & 0xFF000000) >> 24;

        buffer[start]     = is_printable(b1) ? b1 : '.';
        buffer[start + 1] = is_printable(b2) ? b2 : '.';
        buffer[start + 2] = is_printable(b3) ? b3 : '.';
        buffer[start + 3] = is_printable(b4) ? b4 : '.';
    }
    kputchar('\n');
}

void hexdump(const void *ptr, const u32 len) {
#ifdef NO_ANSI
    hexdump_buffer(ptr, len);
#else
    hexdump_ansi(ptr, len);
#endif
}
