// scheduler.c — round-robin process scheduler

#include "kernel/scheduler.h"

#include "kernel/cpu.h"
#include "kernel/dev/memory.h"
#include "kernel/logs.h"
#include "kernel/process.h"

static process_t *procs[MAX_PROCESSES];

volatile process_t *current_proc = nullptr;

extern void context_switch_asm(process_t *next);

[[noreturn]]
void context_switch(process_t *next) {
    u32 pgd_phys = virt_to_phys(next->pgd);
    if (pgd_phys < 0x1000u)
        err("context_switch: pid=%d pgd=%p phys=%p — BAD", next->pid, (void *) next->pgd,
            (void *) pgd_phys);
    context_switch_asm(next);
    __builtin_unreachable();
}

int sched_add(process_t *p) {
    for (u32 i = 0; i < MAX_PROCESSES; i++) {
        if (!procs[i]) {
            p->suspended = 0;
            procs[i]     = p;
            return (int) i;
        }
    }
    err("scheduler: table full (%d processes)", MAX_PROCESSES);
    return -1;
}

[[gnu::noinline]]
int sched_remove(pid_t pid) {
    for (u32 i = 0; i < MAX_PROCESSES; i++) {
        if (procs[i] && procs[i]->pid == pid) {
            procs[i] = nullptr;
            return (int) i;
        }
    }
    err("scheduler: pid=%u not found", pid);
    return -1;
}

process_t *sched_get(pid_t pid) {
    for (u32 i = 0; i < MAX_PROCESSES; i++) {
        if (procs[i] && procs[i]->pid == pid) return procs[i];
    }
    return nullptr;
}

void sched_wake_joiners(pid_t pid, int exit_code) {
    for (u32 i = 0; i < MAX_PROCESSES; i++) {
        process_t *p = procs[i];
        if (p && p->join_target == pid) {
            p->ctx.r[0]    = (u32) exit_code;
            p->join_target = 0;
            p->suspended   = 0;
        }
    }
}

process_t *sched_pick_next(void) {
    if (!current_proc) return nullptr;

    u32 cur = MAX_PROCESSES;

    for (u32 i = 0; i < MAX_PROCESSES; i++) {
        if (procs[i] == current_proc) {
            cur = i;
            break;
        }
    }

    for (u32 j = 1; j <= MAX_PROCESSES; j++) {
        u32        idx = (cur + j) % MAX_PROCESSES;
        process_t *p   = procs[idx];
        if (p && !p->suspended) {
            return p;
        }
    }

    return nullptr;
}

void sched_tick(void) {
    if (!current_proc) return;

    // Wake any processes whose sleep deadline has passed
    u64 now = read_cntpct();
    for (u32 i = 0; i < MAX_PROCESSES; i++) {
        process_t *p = procs[i];
        if (p && p->wake_tick && now >= p->wake_tick) {
            p->wake_tick = 0;
            p->suspended = 0;
        }
    }

    // Locate current process in table
    u32 cur = MAX_PROCESSES;
    for (u32 i = 0; i < MAX_PROCESSES; i++) {
        if (procs[i] == current_proc) {
            cur = i;
            break;
        }
    }
    if (cur == MAX_PROCESSES) return;

    // Round-robin: find the next runnable process, skipping current.
    // we are in IRQ context; the IRQ return path (.Lirq_restore_user) will install
    // current_proc->pgd.
    for (u32 j = 1; j < MAX_PROCESSES; j++) {
        u32        idx = (cur + j) % MAX_PROCESSES;
        process_t *p   = procs[idx];
        if (p && !p->suspended) {
            ((process_t *) current_proc)->suspended = 1;
            current_proc                            = p;
            return;
        }
    }
    // No other runnable process — keep running current
}