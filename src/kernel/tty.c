#include "kernel/tty.h"
#include "kernel/dev/fb.h"
#include "types.h"

#define LINE_BUF_SIZE 256u

static char          g_line_buf[LINE_BUF_SIZE];
static volatile u32  g_line_len = 0;
static volatile bool g_line_ready = false;
static volatile u32  g_line_start = 0;

static volatile bool echo_chars = false;

void tty_putchar(char c) {
    fb_putc(c, FB_WHITE, FB_BLACK);
}

void tty_input(char c) {
    if (c == '\r') c = '\n';

    if (c == '\b' || c == 127) { // backspace / DEL
        if (g_line_len > 0) {
            g_line_len--;
            tty_putchar('\b');
            tty_putchar(' ');
            tty_putchar('\b');
        }
        return;
    }

    if (echo_chars) {
        tty_putchar(c);
    }

    if (g_line_len < LINE_BUF_SIZE - 1)
        g_line_buf[(g_line_start + g_line_len++) % LINE_BUF_SIZE] = c;

    if (c == '\n') g_line_ready = true;
}

u32 tty_readline(char *buf, u32 max) {
    echo_chars = true;
    while (!g_line_ready)
        asm("wfi" ::: "memory"); // spin; IRQs must be enabled by caller

    if (!g_line_len) {
        g_line_ready = false;
        echo_chars = false;
        return 0;
    }

    u32 n = g_line_len < max - 1 ? g_line_len - 1 : max - 1;

    for (u32 i = 0; i < g_line_len; i++)
        buf[i] = g_line_buf[(g_line_start + i) % LINE_BUF_SIZE];
    buf[n] = '\0';

    g_line_start = (g_line_start + g_line_len) % LINE_BUF_SIZE;
    g_line_len = 0;
    g_line_ready = false;
    echo_chars = false;
    return n;
}

char tty_getchar(void) {
    while (!g_line_len)
        asm("wfi" ::: "memory"); // spin; IRQs must be enabled by caller
    char c = g_line_buf[g_line_start++];
    g_line_len--;
    return c;
}

char tty_getchar_nb(void) {
    if (!g_line_len) return '\0';
    char c = g_line_buf[g_line_start++];
    g_line_len--;
    return c;
}
