// process.h — process creation and virtual address space management

#ifndef LEG_PROCESS_H
#define LEG_PROCESS_H

#include "dev/mmu.h"
#include "types.h"

// User virtual address layout
#define PROC_STACK_VA_MB 0x001u                           // stack at VA 0x00100000
#define PROC_STACK_PAGES 256u                             // 1 MB of physical pages
#define PROC_STACK_TOP   ((PROC_STACK_VA_MB + 1u) << 20) // 0x00200000, initial SP

#define PROC_CODE_VA_MB  0x004u                           // code at VA 0x00400000
#define PROC_CODE_PAGES  256u                             // 1 MB of physical pages
#define PROC_CODE_VA     (PROC_CODE_VA_MB << 20)          // 0x00400000, entry point

#define MAX_PROCS 4

typedef u32 pid_t;

// Saved user-mode register context (captured on IRQ preemption).
// Layout is mirrored in boot.s — offsets must stay in sync.
typedef struct {
    u32 r[13]; // r0–r12                                    (ctx+0..+51)
    u32 sp;    // user SP                                   (ctx+52)
    u32 lr;    // user LR                                   (ctx+56)
    u32 pc;    // user PC (lr_irq - 4 at preemption)        (ctx+60)
    u32 cpsr;  // user CPSR (spsr_irq at preemption)        (ctx+64)
} cpu_ctx_t;

struct process {
    pid_t     pid;
    l1_entry *pgd;        // TTBR0 L1 table (PROC_VA_MB entries, user VA only)
    u32       sp;         // initial stack pointer (user VA)
    u32       stack_phys; // physical base of stack (for cleanup)
    u32       code_phys;  // physical base of user code pages (for cleanup)  +16
    cpu_ctx_t ctx;        // saved register state (filled on IRQ preemption)  +20
    u32       suspended;  // non-zero while the process is suspended           +88
};

// Currently executing process (set by process_exec).
extern struct process *current_proc;

// Allocate a process, map its stack, and load the user binary into code pages.
struct process *process_create(void);

// Switch to the process page table and jump to its entry. Never returns.
[[noreturn]]
void process_exec(struct process *p);

[[noreturn]]
void process_exit(struct process *p, int code);

// Called from the timer tick: increments tick count, suspends at 10, resumes at 20.
void sched_tick(void);

#endif // LEG_PROCESS_H
