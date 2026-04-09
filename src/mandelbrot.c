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
#include "mandelbrot.h"

#include "kernel/dev/fb.h"
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

/**
 * @brief Maps an iteration count to an ASCII character.
 * @param value The iteration value (0 to max_iter).
 * @param max_iter The maximum number of iterations.
 * @return A character for visualization.
 */
char cnt2char(int value, int max_iter) {
    const char *symbols = "MW2a_. ";
    int         ns = strlen(symbols);
    // Map the value [0, max_iter] to an index [0, ns-1]
    int idx = (int) ((double) value / max_iter * (ns - 1));
    return symbols[idx];
}

void putpixel(int value, int max_iter, int x, int y) {
    if (value == 0) {
        fb_putpixel(x, y, FB_BLACK);
        return;
    }
    // smooth hue cycling: map escape count to 0..1 and cycle through HSV hues
    double t = (double) value / max_iter;
    double h = t * 360.0 * 4.0; // 4 full hue cycles
    h = h - (int) (h / 360.0) * 360.0;
    double s = 1.0, v = 1.0;
    double c_ = v * s;
    double x_ =
        c_ * (1.0 - (h / 60.0 - (int) (h / 60.0) * 2 > 1.0 ? 2.0 - h / 60.0 + (int) (h / 60.0)
                                                           : h / 60.0 - (int) (h / 60.0)));
    double r1, g1, b1;
    int    hi = (int) (h / 60.0) % 6;
    switch (hi) {
        case 0:
            r1 = c_;
            g1 = x_;
            b1 = 0;
            break;
        case 1:
            r1 = x_;
            g1 = c_;
            b1 = 0;
            break;
        case 2:
            r1 = 0;
            g1 = c_;
            b1 = x_;
            break;
        case 3:
            r1 = 0;
            g1 = x_;
            b1 = c_;
            break;
        case 4:
            r1 = x_;
            g1 = 0;
            b1 = c_;
            break;
        default:
            r1 = c_;
            g1 = 0;
            b1 = x_;
            break;
    }
    u32 r = (u32) (r1 * 255);
    u32 g = (u32) (g1 * 255);
    u32 b = (u32) (b1 * 255);
    fb_putpixel(x, y, (r << 16) | (g << 8) | b);
}

/**
 * @brief Calculates the escape time for a point in the complex plane.
 * @param cr The real part of the complex number c.
 * @param ci The imaginary part of the complex number c.
 * @param max_iter The maximum number of iterations.
 * @return An integer representing how close the point is to the set.
 */
int escape_time(double cr, double ci, int max_iter) {
    double zr = 0.0, zi = 0.0;
    int    iter;

    for (iter = 0; iter < max_iter; ++iter) {
        double zr2 = zr * zr;
        double zi2 = zi * zi;
        if (zr2 + zi2 > 4.0) {
            break;
        }
        double tmp = zr2 - zi2 + cr;
        zi = 2.0 * zr * zi + ci;
        zr = tmp;
    }
    return max_iter - iter;
}

/**
 * @brief Renders the Mandelbrot set as ASCII art to stdout.
 * @param config A pointer to the configuration struct.
 */
void ascii_output(const Config *config) {
    double fwidth = config->ur_x - config->ll_x;
    double fheight = config->ur_y - config->ll_y;

    fb_clear(FB_BLUE);

    for (int y = 0; y < config->height; ++y) {
        for (int x = 0; x < config->width; ++x) {
            double real = config->ll_x + x * fwidth / config->width;
            double imag = config->ur_y - y * fheight / config->height;
            int    iter = escape_time(real, imag, config->max_iter);
            // putchar(cnt2char(iter, config->max_iter));
            putpixel(iter, config->max_iter, x, y);
        }
        // putchar('\n');
    }
}

int mandelbrot(double ll_x, double ll_y, double ur_x, double ur_y) {
    Config config = {.width = FB_WIDTH,
                     .height = FB_HEIGHT,
                     .png = false,
                     .ll_x = ll_x,
                     .ll_y = ll_y,
                     .ur_x = ur_x,
                     .ur_y = ur_y,
                     .max_iter = 255};

    ascii_output(&config);

    return 0;
}