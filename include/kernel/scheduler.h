// scheduler.h — process scheduler (stub)

#ifndef LEG_SCHEDULER_H
#define LEG_SCHEDULER_H

#include "process.h"

typedef struct {
    struct process procs[MAX_PROCS];
    pid_t          current_pid;
} scheduler_t;

scheduler_t scheduler;

#endif // LEG_SCHEDULER_H
