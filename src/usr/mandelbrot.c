//
// Created by Romir Kulshrestha on 09/04/2026.
//
/**
 * @file mandelbrot.c
 * @brief Generates Mandelbrot set visualizations in ASCII or for gnuplot.
 *
 * A modern C implementation for a cross-language comparison project.
 * It parses command-line arguments in the format key=value.
 *
 * Compilation:
 * gcc -o mandelbrot mandelbrot.c -lm -O3
 *
 * Usage:
 * ./mandelbrot
 * ./mandelbrot width=120 ll_x=-0.75 ll_y=0.1 ur_x=-0.74 ur_y=0.11
 * ./mandelbrot png=1 width=800 height=600 > mandelbrot.dat
 */
#include "usr/mandelbrot.h"

#include "libc/display.h"
#include "libc/string.h"

typedef struct {
    int    width;
    int    height;
    bool   png;
    double ll_x;
    double ll_y;
    double ur_x;
    double ur_y;
    int    max_iter;
} Config;

char cnt2char(int value, int max_iter) {
    const char *symbols = "MW2a_. ";
    int         ns = strlen(symbols);
    int         idx = (int) ((double) value / max_iter * (ns - 1));
    return symbols[idx];
}

// Blue-gold palette: navy → royal blue → sky blue → white → gold → orange → dark brown → navy
typedef struct {
    double t;
    u8     r, g, b;
} ColorStop;
static const ColorStop palette[] = {
    {0.00, 0, 2, 80},      {0.30, 20, 90, 210}, {0.50, 80, 170, 255},
    {0.65, 255, 255, 255}, {0.75, 255, 210, 0}, {0.85, 255, 130, 0},
    {0.95, 80, 10, 0},     {1.00, 0, 2, 80}, // wraps back to navy
};

static u32 palette_lookup(double t) {
    constexpr int N = sizeof(palette) / sizeof(palette[0]) - 1;
    for (int i = 0; i < N; i++) {
        if (t <= palette[i + 1].t) {
            double f = (t - palette[i].t) / (palette[i + 1].t - palette[i].t);
            u32    r = (u32) (palette[i].r + f * ((int) palette[i + 1].r - palette[i].r));
            u32    g = (u32) (palette[i].g + f * ((int) palette[i + 1].g - palette[i].g));
            u32    b = (u32) (palette[i].b + f * ((int) palette[i + 1].b - palette[i].b));
            return (r << 16) | (g << 8) | b;
        }
    }
    return 0;
}

// Natural log via IEEE 754 bit extraction + atanh series for mantissa in [1,2).
// Accurate to ~7 decimal places
// Technique: https://math.stackexchange.com/a/977836
static double d_log(double x) {
    union {
        double d;
        u64    u;
    } b;
    b.d = x;
    int e = (int) ((b.u >> 52) & 0x7FFu) - 1023;
    b.u = (b.u & 0x000FFFFFFFFFFFFFULL) | 0x3FF0000000000000ULL; // m in [1,2)
    double t = (b.d - 1.0) / (b.d + 1.0);                        // atanh argument in [0, 1/3)
    double t2 = t * t;
    double ln_m =
        2.0 * t * (1.0 + t2 * (1.0 / 3.0 + t2 * (1.0 / 5.0 + t2 * (1.0 / 7.0 + t2 / 9.0))));
    return e * 0.6931471805599453 + ln_m;
}

// Returns smooth (continuous) escape count, or 0.0 for in-set.
// Smooth = iter + 1 - d_log(d_log(|z|)) / d_log(2), removing integer banding.
[[gnu::pure]]
static double escape_time(double cr, double ci, int max_iter) {
    double zr = 0.0, zi = 0.0;
    for (int iter = 0; iter < max_iter; ++iter) {
        double zr2 = zr * zr, zi2 = zi * zi;
        if (zr2 + zi2 > 4.0) {
            double log_zn = d_log(zr2 + zi2) * 0.5;         // ln(|z|)
            double nu = d_log(log_zn) / 0.6931471805599453; // log2(d_log(|z|))
            return (double) iter + 1.0 - nu;
        }
        double tmp = zr2 - zi2 + cr;
        zi = 2.0 * zr * zi + ci;
        zr = tmp;
    }
    return 0.0;
}

static void putpixel(double smooth, int x, int y) {
    if (smooth == 0.0) {
        fb_putpixel(x, y, FB_BLACK);
        return;
    }
    // Cycle the palette every 32 iterations
    double t = smooth / 32.0;
    t = t - (int) t; // fractional part → [0, 1)
    fb_putpixel(x, y, palette_lookup(t));
}

void ascii_output(const Config *config) {
    double     fwidth = config->ur_x - config->ll_x;
    double     fheight = config->ur_y - config->ll_y;
    const auto max_iter = config->max_iter;

    fb_clear(FB_BLACK);

    for (int y = 0; y < config->height; ++y) {
        for (int x = 0; x < config->width; ++x) {
            double real = config->ll_x + x * fwidth / config->width;
            double imag = config->ur_y - y * fheight / config->height;
            double smooth = escape_time(real, imag, max_iter);
            putpixel(smooth, x, y);
        }
    }
}

[[gnu::noinline]]
int mandelbrot(double min_re, double min_im, double max_re, double max_im) {
    Config config = {.width = FB_WIDTH,
                     .height = FB_HEIGHT,
                     .png = false,
                     .ll_x = min_re,
                     .ll_y = min_im,
                     .ur_x = max_re,
                     .ur_y = max_im,
                     .max_iter = 1000};

    ascii_output(&config);

    return 0;
}