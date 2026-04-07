//
// Created by Romir Kulshrestha on 07/04/2026.
//

#ifndef LEG_SCHEDULER_H
#define LEG_SCHEDULER_H
#include "types.h"

typedef u32 pid_t;

typedef u32 (entry_fn)(u8 argc, const char **argv);

typedef enum {
    Stopped,
    Running,
    Blocked,
} ProcessState;

struct Process {
    pid_t pid;
    pid_t ppid;
    entry_fn *entry;
    ProcessState state;
};

#endif //LEG_SCHEDULER_H
