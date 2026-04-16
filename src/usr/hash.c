#include "usr/hash.h"

#include "libc/cstring.h"
#include "libc/display.h"
#include "libc/stdio.h"
#include "libc/stdlib.h"
#include "syscall.h"
#include "types.h"

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
        printf("%s\t\t%u\n", name, size);
    }
    printf("%u blobs\n", count);
    return true;
}

static bool cmd_help(int, char **);

static const Command commands[] = {
    {"clear", cmd_clear}, {"echo", cmd_echo}, {"exit", cmd_exit},
    {"help", cmd_help},   {"ls", cmd_ls},     {"sleep", cmd_sleep},
};

static bool cmd_help(int, char **) {
    for (u32 i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
        printf("%s\n", commands[i].name);
    }
    return true;
}

static bool exec(char *name) {
    auto pid = sys_fork();
    if (pid == 0) {
        sys_exec(name);
        printf("exec: failed to execute '%s'\n", name);
        return false;
    }
    sys_join(pid);
    return true;
}

typedef struct [[gnu::packed]] {
    u32 offset;      // byte offset of blob data from FS image start
    u32 size;        // blob size in bytes
    u32 name_offset; // byte offset of name within the name_data region
    u16 name_size;   // name length in bytes (no null terminator on disk)
    u16 flags;
} fs_blob_t;

int main(int, char **) {
    char  buf[256];
    char *argv[MAX_ARGS];

    loop {
        printf("hash # ");
        readline(buf, sizeof(buf));

        int argc = str_split(buf, argv, MAX_ARGS);
        if (argc == 0) continue;

        bool found = false;
        for (u32 i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
            if (strcmp(argv[0], commands[i].name) == 0) {
                found = true;
                if (!commands[i].fn(argc, argv)) return 0;
                break;
            }
        }

        if (!found) {
            auto blob_count = sys_fs_blob_count();
            for (u32 i = 0; i < blob_count; i++) {
                char       name_buf[256];
                u32        size = 0;
                const auto blob =
                    (fs_blob_t *) sys_fs_blob_info(i, name_buf, sizeof(name_buf), &size);
                if (blob->flags == 0x0001u && strncmp(name_buf, argv[0], size) == 0) {
                    found = exec(name_buf);
                    if (found) break;
                }
            }
        }
        if (!found) {
            printf("Unknown command: %s\n", argv[0]);
        }
    }
}
