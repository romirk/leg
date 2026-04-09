#include "kernel/main.h"

#include "utils.h"
#include "kernel/dev/fb.h"
#include "kernel/dev/rtc.h"
#include "kernel/dev/rng.h"
#include "kernel/tty.h"
#include "libc/stdio.h"
#include "libc/string.h"

#define MATRIX_COLS  FB_COLS
#define MATRIX_ROWS  FB_ROWS
#define TRAIL_LEN    16
#define BRIGHT_GREEN 0x00CCFFCCu
#define MED_GREEN    0x0000CC00u
#define DIM_GREEN    0x00006600u
#define FAINT_GREEN  0x00003300u

static struct {
    u32  y;      // current head row
    u32  speed;  // frames between advances
    u32  tick;   // frame counter
    u32  len;    // trail length
    bool active; // whether the drop is on-screen
} drops[MATRIX_COLS];

static char grid[MATRIX_COLS][MATRIX_ROWS];

static char rand_char(void) {
    u32 r = rand_below(63);
    if (r < 10) return '0' + r;
    if (r < 36) return 'A' + (r - 10);
    return '!' + (r - 36);
}

static u32 fade_color(u32 dist) {
    if (dist == 0) return BRIGHT_GREEN;
    if (dist == 1) return FB_GREEN;
    if (dist <= 3) return MED_GREEN;
    if (dist <= 6) return DIM_GREEN;
    if (dist <= 10) return FAINT_GREEN;
    return FB_BLACK;
}

static void init_drop(u32 col) {
    drops[col].y = 0;
    drops[col].speed = 1 + rand32() % 3;
    drops[col].tick = 0;
    drops[col].len = 8 + rand32() % (TRAIL_LEN + 1);
    drops[col].active = true;
}

void matrix(void) {
    rng_seed((u32) rtc_ticks());
    fb_clear(FB_BLACK);

    // stagger initial drops
    for (u32 c = 0; c < MATRIX_COLS; c++) {
        init_drop(c);
        drops[c].y = rand32() % MATRIX_ROWS;
        drops[c].active = (rand32() % 3) != 0;
        for (u32 r = 0; r < MATRIX_ROWS; r++)
            grid[c][r] = rand_char();
    }

    loop {
        // ReSharper disable once CppDFAEndlessLoop
        for (u32 c = 0; c < MATRIX_COLS; c++) {
            if (!drops[c].active) {
                if ((rand32() % 40) == 0) init_drop(c);
                continue;
            }

            drops[c].tick++;
            if (drops[c].tick < drops[c].speed) continue;
            drops[c].tick = 0;

            // advance head
            drops[c].y++;
            u32 head = drops[c].y;

            // deactivate once the trail has scrolled fully off-screen
            if (head > MATRIX_ROWS + drops[c].len) {
                drops[c].active = false;
                continue;
            }

            // mutate a random character somewhere in the trail
            if (head >= 2 && head <= MATRIX_ROWS) {
                u32 mr = head - 2 + rand32() % 3;
                if (mr < MATRIX_ROWS) grid[c][mr] = rand_char();
            }

            // draw new char at head
            if (head < MATRIX_ROWS) {
                grid[c][head] = rand_char();
                fb_putc_at(c, head, grid[c][head], BRIGHT_GREEN, FB_BLACK);
            }

            // redraw trail with fading colours
            for (u32 d = 1; d <= drops[c].len + 4; d++) {
                if (head < d) break;
                u32 r = head - d;
                if (r >= MATRIX_ROWS) continue;
                u32 color = fade_color(d);
                fb_putc_at(c, r, grid[c][r], color, FB_BLACK);
            }

            // erase the tail cell
            u32 erase = (head > drops[c].len + 4) ? head - drops[c].len - 4 : 0;
            if (erase < MATRIX_ROWS && head > drops[c].len + 4)
                fb_putc_at(c, erase, ' ', FB_BLACK, FB_BLACK);
        }

        if (getchar_nonblocking() == 'q') break;
        delay_us(50000);
    }
}

int main() {
    char buf[256];
    do {
        printf("hash # ");
        readline(buf, sizeof(buf));

        if (strcmp(buf, "exit") == 0) {
            break;
        }
        if (strncmp(buf, "echo ", 5) == 0) {
            printf("%s\n", buf + 5);
        } else if (strncmp(buf, "matrix", 5) == 0) {
            matrix();
        } else if (strncmp(buf, "clear", 5) == 0) {
            fb_clear(FB_BLACK);
        }

    } while (true);
    return 0;
}
