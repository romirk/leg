//
// fb.h — ramfb framebuffer (QEMU paravirtual display)
//

#ifndef LEG_FB_H
#define LEG_FB_H

#include "types.h"

#define FB_WIDTH   800
#define FB_HEIGHT  600
#define FB_BPP     4          // XRGB8888
#define FB_STRIDE  (FB_WIDTH * FB_BPP)

// colours (XRGB8888)
#define FB_BLACK   0x00000000u
#define FB_WHITE   0x00FFFFFFu
#define FB_GREEN   0x0000FF00u
#define FB_RED     0x00FF0000u
#define FB_BLUE    0x000000FFu
#define FB_GREY    0x00888888u
#define FB_DGREY   0x00333333u

// font metrics
#define FONT_W  8
#define FONT_H  8
#define FB_COLS (FB_WIDTH  / FONT_W)
#define FB_ROWS (FB_HEIGHT / FONT_H)

// init — allocates physical memory, configures ramfb via fw-cfg
void fb_init(void);

// pixel-level
void fb_putpixel(u32 x, u32 y, u32 color);

void fb_fill_rect(u32 x, u32 y, u32 w, u32 h, u32 color);

void fb_clear(u32 color);

// text — uses internal cursor, scrolls
void fb_putc(char c, u32 fg, u32 bg);

void fb_putc_at(u32 col, u32 row, char c, u32 fg, u32 bg);

void fb_print(const char *s, u32 fg, u32 bg);

#endif //LEG_FB_H
