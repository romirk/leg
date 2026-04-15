// process.h — process creation and virtual address space management

#ifndef LEG_PROCESS_H
#define LEG_PROCESS_H

#include "dev/mmu.h"
#include "types.h"

// User virtual address layout
constexpr u32 PROC_USER_VA_TOP = (1 << (32 - TTBCR_N)); // 2^(32 - N)

// Stack: grows downward from just below the top, one guard page left unmapped
#define PROC_STACK_GUARD      0x1000u // one unmapped page at ceiling
#define PROC_STACK_INIT_PAGES 4u      // pages mapped at process creation
constexpr u32 PROC_STACK_TOP = PROC_USER_VA_TOP - PROC_STACK_GUARD; // initial SP

// Code: starts at 1MB so null-deref (0x0) faults cleanly
#define PROC_CODE_VA_MB 0x001u
constexpr u32 PROC_CODE_VA  = (PROC_CODE_VA_MB << MB_SHIFT); // 0x00100000, entry point
constexpr u32 PROC_CODE_MAX = (1u << MB_SHIFT);              // max code size (1 MB)

// Heap: immediately follows the code region, grows upward
constexpr u32 PROC_HEAP_START = (PROC_CODE_VA + PROC_CODE_MAX);

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

typedef struct process {
    pid_t     pid;       //                                                                     +0
    l1_entry *pgd;       // TTBR0 L1 table                                                      +4
    uptr      sp;        // current stack pointer (user VA)                                     +8
    uptr      entry;     // VA entry point (from LLF header), non-zero denotes valid process    +12
    cpu_ctx_t ctx;       // saved register state (filled on IRQ preemption)                     +16
    u32       suspended; // non-zero while the process is suspended                             +84
    // ── fields below are not accessed from assembly ──────────────────────────
    u64       wake_tick;   // CNTPCT deadline for sleep; 0 = not sleeping
    pid_t     join_target; // PID this process is waiting on (0 = not waiting)
    l2_entry *stack_pt;    // L2 table covering the stack's current L1 slot
    u32       stack_pages; // number of 4KB stack pages currently mapped
    uptr      heap_end;    // current top of heap (user VA), initially PROC_HEAP_START
} process_t;

// Allocate a process, map its stack and code, and load the named binary from the filesystem.
struct process *process_create(const char *name);

// Switch to the process page table and jump to its entry. Never returns.
[[noreturn]]
void process_exec(struct process *p);

[[noreturn]]
void process_exit(pid_t pid, int code);

// Clone the current process. The child is added to the scheduler (suspended) and
// will return 0 from fork; the parent receives the child's PID. Returns nullptr on OOM.
process_t *process_fork(u32 lr_svc, u32 sp_usr, u32 cpsr);

// Map one additional 4KB page onto the bottom of the process stack.
// Returns false on OOM.
bool process_add_page(struct process *p);

void process_replace(pid_t pid, char *name);

#endif // LEG_PROCESS_H
