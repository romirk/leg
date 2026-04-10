// display.h — userspace framebuffer API
//
// Inline wrappers around the sys_fb_* SVCs. Mirrors the constants in
// kernel/dev/fb.h so that usr/ code never needs to include kernel headers.

#ifndef LEG_DISPLAY_H
#define LEG_DISPLAY_H

#include "syscall.h"
#include "types.h"

// ─── Dimensions ──────────────────────────────────────────────────────────────

#define FB_WIDTH  800
#define FB_HEIGHT 600
#define FB_BPP    4
#define FB_STRIDE (FB_WIDTH * FB_BPP)

#define FONT_W  8
#define FONT_H  8
#define FB_COLS (FB_WIDTH / FONT_W)
#define FB_ROWS (FB_HEIGHT / FONT_H)

// ─── Colours (XRGB8888) ──────────────────────────────────────────────────────

#define FB_BLACK 0x00000000u
#define FB_WHITE 0x00FFFFFFu
#define FB_GREEN 0x0000FF00u
#define FB_RED   0x00FF0000u
#define FB_BLUE  0x000000FFu
#define FB_GREY  0x00888888u
#define FB_DGREY 0x00333333u

// ─── Drawing ─────────────────────────────────────────────────────────────────

// Fill the entire framebuffer with color.
static inline void fb_clear(u32 color) {
    sys_fb_clear(color);
}

// Set a single pixel at (x, y).
static inline void fb_putpixel(u32 x, u32 y, u32 color) {
    sys_fb_putpixel(x, y, color);
}

// Draw character c at text-grid position (col, row) with fg/bg colors.
static inline void fb_putc_at(u32 col, u32 row, char c, u32 fg, u32 bg) {
    sys_fb_putc((col << 16) | row, c, fg, bg);
}

#endif // LEG_DISPLAY_H
