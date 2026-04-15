// scheduler.h — round-robin process scheduler

#ifndef LEG_SCHEDULER_H
#define LEG_SCHEDULER_H
#include "process.h"

#define MAX_PROCESSES 8

extern volatile process_t *current_proc; // the currently running process (nullptr if idle)

// Add a process to the scheduler run queue. Returns slot index, -1 if full.
int sched_add(process_t *p);

// Remove a process from the run queue by PID. Returns slot index, -1 if not found.
int sched_remove(pid_t pid);

// Find a process by PID. Returns nullptr if not found.
process_t *sched_get(pid_t pid);
process_t *sched_pick_next(void);

// Wake every process blocked in join() waiting for pid. Delivers exit_code into r0.
void sched_wake_joiners(pid_t pid, int exit_code);
// Called from the timer tick (IRQ context). Preempts the current process and
// switches to the next runnable one (round-robin). No-ops if only one process.
void sched_tick(void);

// assembly routine
void context_switch(process_t *next);

#endif // LEG_SCHEDULER_H