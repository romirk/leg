// process.c — process creation and execution

#include "kernel/process.h"
#include "kernel/mmu.h"
#include "kernel/mm.h"
#include "kernel/logs.h"
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
void process_exit(struct process *p, int code) {
    info("process: pid=%d exited with code %d", p->pid, code);
    // TODO: reclaim resources, wake scheduler
    poweroff();
}

[[noreturn]]
void process_exec(struct process *p) {
    info("process: exec pid=%d", p->pid);
    current_proc = p;

    // switch to process page table — kernel VA stays mapped, so we survive this
    mmu_set_proc_table(p->pgd);

    int code;
    asm volatile("mov sp, %1 \n\t" // switch to process stack VA
                 "blx %2     \n\t" // call entry(); r0 = return code on return
                 "mov %0, r0"      // capture return value
                 : "=r"(code)
                 : "r"(p->sp), "r"(p->entry)
                 : "r0", "r1", "r2", "r3", "lr");
    process_exit(p, code);
}
