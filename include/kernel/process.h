// process.h — process creation and virtual address space management

#ifndef LEG_PROCESS_H
#define LEG_PROCESS_H

#include "types.h"
#include "kernel/mmu.h"

// User virtual address layout
#define PROC_STACK_VA_MB 0x001u                          // 1MB at VA 0x00100000
#define PROC_STACK_PAGES 256u                            // 1MB of physical pages
#define PROC_STACK_TOP   ((PROC_STACK_VA_MB + 1u) << 20) // 0x00200000, initial SP

typedef u32 pid_t;

typedef int (*proc_entry_t)(void);

struct process {
    pid_t              pid;
    translation_table *pgd; // L1 table (cloned from kernel)
    u32                sp;  // initial stack pointer (user VA)
    proc_entry_t       entry;
    u32                stack_phys; // physical base of stack (for cleanup)
};

// Currently executing process (set by process_exec).
extern struct process *current_proc;

// Allocate a process and map its stack at PROC_STACK_VA_MB.
struct process *process_create(proc_entry_t entry);

// Switch to the process page table and jump to its entry. Never returns.
[[noreturn]]
void process_exec(struct process *p);

[[noreturn]]
void process_exit(struct process *p, int code);

#endif // LEG_PROCESS_H
