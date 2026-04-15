// fb.c — ramfb framebuffer driver + 8x8 text console

#include "kernel/dev/fb.h"
#include "kernel/dev/fonts.h"
#include "kernel/dev/fwcfg.h"
#include "kernel/dev/mmu.h"
#include "kernel/logs.h"
#include "kernel/mem/alloc.h"
#include "libc/bswap.h"

// ramfb configuration written to fw-cfg "etc/ramfb" — all fields big-endian
struct [[gnu::packed]] ramfb_cfg {
    u64 addr;   // physical address of the framebuffer
    u32 fourcc; // pixel format (DRM_FORMAT_XRGB8888)
    u32 flags;  // reserved, must be 0
    u32 width;
    u32 height;
    u32 stride; // bytes per row
};

// DRM_FORMAT_XRGB8888 = little-endian fourcc "XR24"
#define DRM_FORMAT_XRGB8888 0x34325258u

#define FONT sans_data

static volatile u32 *fb_base;
static u32           fb_phys;

static u32 cursor_col;
static u32 cursor_row;

void fb_init(void) {
    constexpr u32 fb_bytes = FB_WIDTH * FB_HEIGHT * FB_BPP;
    constexpr u32 fb_pages = (fb_bytes + PAGE_SIZE - 1) / PAGE_SIZE;
    fb_phys = mm_page_alloc_n(fb_pages);
    if (!fb_phys) {
        warn("fb: page alloc failed");
        return;
    }

    // identity-map all sections covering the framebuffer
    const u32 first_section = fb_phys >> 20;
    const u32 last_section = (fb_phys + fb_bytes - 1) >> 20;
    for (u32 s = first_section; s <= last_section; s++)
        mmu_map_identity(s, false);

    fb_base = (volatile u32 *) fb_phys;
    fb_clear(FB_BLACK);

    const i32 sel = fwcfg_find("etc/ramfb");
    if (sel < 0) {
        warn("fb: etc/ramfb not found");
        return;
    }

    static struct ramfb_cfg cfg;
    cfg.addr = bswap64(fb_phys);
    cfg.fourcc = bswap32(DRM_FORMAT_XRGB8888);
    cfg.flags = 0;
    cfg.width = bswap32(FB_WIDTH);
    cfg.height = bswap32(FB_HEIGHT);
    cfg.stride = bswap32(FB_STRIDE);

    fwcfg_dma_write((u16) sel, &cfg, sizeof(cfg));

    cursor_col = 0;
    cursor_row = 0;
    info("fb: " STR(FB_WIDTH) "x" STR(FB_HEIGHT) " @ 0x%p", (void *) fb_phys);
}

void fb_putpixel(const u32 x, const u32 y, const u32 color) {
    if (x < FB_WIDTH && y < FB_HEIGHT) fb_base[y * FB_WIDTH + x] = color;
}

void fb_fill_rect(const u32 x, const u32 y, const u32 w, const u32 h, const u32 color) {
    for (u32 row = y; row < y + h && row < FB_HEIGHT; row++)
        for (u32 col = x; col < x + w && col < FB_WIDTH; col++)
            fb_base[row * FB_WIDTH + col] = color;
}

void fb_clear(const u32 color) {
    const u32 total = FB_WIDTH * FB_HEIGHT;
    for (u32 i = 0; i < total; i++)
        fb_base[i] = color;
    cursor_col = 0;
    cursor_row = 0;
}

static void draw_glyph(const u32 cx, const u32 cy, const u8 ch, const u32 fg, const u32 bg) {
    const u8 *glyph = &FONT[ch * 8];
    u32       px = cx * FONT_W;
    u32       py = cy * FONT_H;
    for (u32 row = 0; row < FONT_H; row++) {
        u8 bits = glyph[row];
        for (u32 col = 0; col < FONT_W; col++) {
            u32 color = (bits & (0x80 >> col)) ? fg : bg;
            fb_base[(py + row) * FB_WIDTH + px + col] = color;
        }
    }
}

static void scroll_up(const u32 bg) {
    u32 row_pixels = FONT_H * FB_WIDTH;
    u32 total = (FB_HEIGHT - FONT_H) * FB_WIDTH;
    for (u32 i = 0; i < total; i++)
        fb_base[i] = fb_base[i + row_pixels];
    u32 start = total;
    for (u32 i = start; i < start + row_pixels; i++)
        fb_base[i] = bg;
}

void fb_putc(const char c, const u32 fg, const u32 bg) {
    if (!fb_base) return;

    if (c == '\n' || c == '\r') {
        cursor_col = 0;
        cursor_row++;
    } else if (c == '\b') {
        if (cursor_col > 0) {
            cursor_col--;
            draw_glyph(cursor_col, cursor_row, ' ', fg, bg);
        }
    } else if (c == '\t') {
        cursor_col = (cursor_col + 4) & ~3u;
    } else {
        draw_glyph(cursor_col, cursor_row, c, fg, bg);
        cursor_col++;
    }

    if (cursor_col >= FB_COLS) {
        cursor_col = 0;
        cursor_row++;
    }
    while (cursor_row >= FB_ROWS) {
        scroll_up(bg);
        cursor_row--;
    }
}

void fb_putc_at(const u32 col, const u32 row, const char c, const u32 fg, const u32 bg) {
    if (col < FB_COLS && row < FB_ROWS) draw_glyph(col, row, (u8) c, fg, bg);
}

void fb_print(const char *s, const u32 fg, const u32 bg) {
    while (*s)
        fb_putc(*s++, fg, bg);
}
