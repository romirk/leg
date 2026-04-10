// process.c — process creation and execution

#include "kernel/process.h"
#include "kernel/mem/mmu.h"
#include "kernel/mem/alloc.h"
#include "kernel/logs.h"
#include "kernel/cpu.h"
#include "utils.h"

static pid_t next_pid = 1;

struct process *current_proc = nullptr;

struct process *process_create(proc_entry_t entry) {
    struct process *p = kzalloc(sizeof(*p));
    if (!p) {
        err("process: OOM for struct");
        return nullptr;
    }

    // clone kernel L1 table — process inherits all kernel/device mappings
    p->pgd = mmu_alloc_proc_table();
    if (!p->pgd) {
        err("process: no free L1 table slots");
        kfree(p);
        return nullptr;
    }

    p->stack_phys = (u32) mm_page_alloc_n(PROC_STACK_PAGES);
    if (!p->stack_phys) {
        err("process: OOM for stack");
        mmu_free_proc_table(p->pgd);
        kfree(p);
        return nullptr;
    }

    // identity-map the stack section in the kernel table (so kernel can reach it)
    mmu_map_identity(p->stack_phys >> 20, false);

    // map stack at the process's user VA
    mmu_map_section(p->pgd, PROC_STACK_VA_MB, p->stack_phys >> 20, false);

    p->pid = next_pid++;
    p->sp = PROC_STACK_TOP - 16; // 16-byte aligned, safety gap from top
    p->entry = entry;

    info("process: pid=%d entry=0x%p stack phys=0x%p va=0x%p", p->pid, (u32) p->entry,
         p->stack_phys, PROC_STACK_VA_MB << 20);

    return p;
}

[[noreturn]]
void process_exit([[maybe_unused]] const struct process *p, [[maybe_unused]] const int code) {
    info("process: pid=%d exited with code %d", p->pid, code);
    // TODO: reclaim resources, wake scheduler
    poweroff();
}

[[gnu::naked]]
void sys_exit_stub(void) {
    asm volatile("svc #0");
}

[[noreturn]]
void process_exec(struct process *p) {
    current_proc = p;

    mmu_set_proc_table(p->pgd);

    psr_t s = {
        .M = usr,   // User mode
        .I = false, // Enable interrupts for userland
    };

    write_spsr(s); // Set up SPSR_svc

    asm volatile("cps #31          \n\t" // System Mode
                 "mov sp, %0       \n\t" // Set sp_usr
                 "mov lr, %1       \n\t" // Set lr_usr (exit stub)
                 "cps #19          \n\t" // Back to SVC Mode

                 "mov lr, %2       \n\t" // entry point into lr_svc
                 "movs pc, lr      \n\t" // Mode switch (PC=lr, CPSR=SPSR)
                 :
                 : "r"(p->sp), "r"(sys_exit_stub), "r"(p->entry)
                 : "memory");

    __builtin_unreachable();
}
