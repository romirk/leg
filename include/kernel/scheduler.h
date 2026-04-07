// scheduler.h — process scheduler (stub)

#ifndef LEG_SCHEDULER_H
#define LEG_SCHEDULER_H
#include "types.h"

typedef u32 pid_t;

typedef u32(entry_fn)(u8 argc, const char **argv);

typedef enum {
    Stopped,
    Running,
    Blocked,
} ProcessState;

// scheduler-visible process descriptor
struct Process {
    pid_t        pid;   // unique process id
    pid_t        ppid;  // parent process id
    entry_fn    *entry; // entry point
    ProcessState state;
};

#endif // LEG_SCHEDULER_H
