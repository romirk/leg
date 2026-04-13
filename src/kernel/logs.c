// logs.c — kernel-only print functions that write directly to UART.
// Used by the log macros in logs.h; bypasses tty/fb entirely.

#include "kernel/logs.h"
#include "kernel/dev/uart.h"
#include "libc/stdlib.h"

#include <stdarg.h>

static void kputchar(const char c) {
    uart_putchar(c);
}

int kprint(const char *s) {
    const char *p = s;
    while (*p) kputchar(*p++);
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

void kprintf(const char *fmt, ...) {
    va_list args;
    va_start(args);

    char c;
    while ((c = *fmt++)) {
        if (c != '%') { kputchar(c); continue; }
        switch (*fmt++) {
        case '%': kputchar('%');                              break;
        case 's': kprint(va_arg(args, char *));              break;
        case 'c': kputchar((char) va_arg(args, int));        break;
        case 'd':
        case 'i': kprint_i32(va_arg(args, i32));             break;
        case 'u': kprint_base(va_arg(args, u32), 10);        break;
        case 'x': kprint_base(va_arg(args, u32), 16);        break;
        case 'o': kprint_base(va_arg(args, u32), 8);         break;
        case 'b': kprint_base(va_arg(args, u32), 2);         break;
        case 't': kprint(va_arg(args, int) ? "true":"false"); break;
        case 'p': kprint_ptr(va_arg(args, void *));          break;
        default:  kprint("?%"); kputchar(*(fmt - 1));        break;
        }
    }
    va_end(args);
}