#include "usr/hash.h"

#include "libc/cstring.h"
#include "libc/display.h"
#include "libc/stdio.h"
#include "libc/stdlib.h"
#include "libc/time.h"
#include "syscall.h"
#include "types.h"
#include "usr/mandelbrot.h"
#include "usr/matrix.h"

#define MAX_ARGS 16

typedef bool (*cmd_fn)(int argc, char **argv);

typedef struct {
    const char *name;
    cmd_fn      fn;
} Command;

static bool cmd_exit(int, char **) {
    return false;
}

static bool cmd_echo(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        if (i > 1) putchar(' ');
        printf("%s", argv[i]);
    }
    putchar('\n');
    return true;
}

static bool cmd_matrix(int, char **) {
    matrix();
    return true;
}

static bool cmd_clear(int, char **) {
    fb_clear(FB_BLACK);
    return true;
}

static bool cmd_sleep(int argc, char **argv) {
    if (argc < 2) {
        printf("usage: sleep <s>\n");
        return true;
    }
    u32 us = (u32) (atof(argv[1]) * 1000000.0);
    sys_sleep(us);
    return true;
}

[[gnu::noinline]]
static bool cmd_ls(int, char **) {
    u32  count = sys_fs_blob_count();
    char name[64];
    for (u32 i = 0; i < count; i++) {
        u32 size = 0;
        sys_fs_blob_info(i, name, sizeof(name), &size);
        printf("%s  %u\n", name, size);
    }
    printf("%u blobs\n", count);
    return true;
}

static bool cmd_brot(int argc, char **argv) {
    double min_re, min_im, max_re, max_im;

    if (argc >= 5) {
        min_re = atof(argv[1]);
        min_im = atof(argv[2]);
        max_re = atof(argv[3]);
        max_im = atof(argv[4]);
    } else {
        constexpr double c_re = -.74364085;
        constexpr double c_im = .13182733;
        constexpr double d_re = .00012068;
        constexpr double d_im = d_re * (600.0 / 800.0);
        min_re = c_re - d_re / 2;
        max_re = c_re + d_re / 2;
        min_im = c_im - d_im / 2;
        max_im = c_im + d_im / 2;
    }

    u64 t0 = get_ticks();
    mandelbrot(min_re, min_im, max_re, max_im);
    u64 dt = get_ticks() - t0;
    u32 freq = cntfrq();
    u32 secs = (u32) (dt / freq);
    u32 ms = (u32) ((dt % freq) * 1000 / freq);
    printf("brot: %d.", secs);
    if (ms < 100) putchar('0');
    if (ms < 10) putchar('0');
    printf("%ds\n", ms);
    return true;
}

static bool cmd_help(int, char **);

static const Command commands[] = {
    {"brot", cmd_brot}, {"clear", cmd_clear}, {"echo", cmd_echo},     {"exit", cmd_exit},
    {"help", cmd_help}, {"ls", cmd_ls},       {"matrix", cmd_matrix}, {"sleep", cmd_sleep},
};

static bool cmd_help(int, char **) {
    for (u32 i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
        printf("%s\n", commands[i].name);
    }
    return true;
}

void hash_run(void) {
    char  buf[256];
    char *argv[MAX_ARGS];

    while (true) {
        printf("hash # ");
        readline(buf, sizeof(buf));

        int argc = str_split(buf, argv, MAX_ARGS);
        if (argc == 0) continue;

        bool found = false;
        for (u32 i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
            if (strcmp(argv[0], commands[i].name) == 0) {
                found = true;
                if (!commands[i].fn(argc, argv)) return;
                break;
            }
        }
        if (!found) printf("unknown command: %s\n", argv[0]);
    }
}
