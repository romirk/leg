// process.c — process creation and execution

#include "kernel/cpu.h"
#include "kernel/dev/mmu.h"
#include "kernel/logs.h"
#include "kernel/mem/alloc.h"
#include "kernel/process.h"
#include "kernel/user_image.h"
#include "libc/builtins.h"
#include "utils.h"

static pid_t next_pid = 0;

struct process *current_proc = nullptr;

struct process *process_create(void) {
    struct process *p = kmalloc_aligned(sizeof(*p), 0x4000);
    if (!p) {
        err("process: OOM for struct");
        return nullptr;
    }

    p->pgd = mmu_alloc_proc_table();
    if (!p->pgd) {
        err("process: OOM for L1 table");
        kfree(p);
        return nullptr;
    }

    p->stack_phys = mm_page_alloc_aligned(PROC_STACK_PAGES, PROC_STACK_PAGES);
    if (!p->stack_phys) {
        err("process: OOM for stack");
        mmu_free_proc_table(p->pgd);
        kfree(p);
        return nullptr;
    }

    p->code_phys = mm_page_alloc_aligned(PROC_CODE_PAGES, PROC_CODE_PAGES);
    if (!p->code_phys) {
        err("process: OOM for code");
        mm_page_free_n(p->stack_phys, PROC_STACK_PAGES);
        mmu_free_proc_table(p->pgd);
        kfree(p);
        return nullptr;
    }

    // Temporarily identity-map the code pages so the kernel can write to them.
    mmu_map_identity(p->code_phys >> 20, false);

    // Zero the code pages so BSS (not covered by the binary) is clean.
    memset((void *) p->code_phys, 0, PROC_CODE_PAGES * PAGE_SIZE);

    // Copy the user binary into the code pages.
    const u32 image_size = (u32)(user_image_end - user_image_start);
    ASSERT(image_size <= PROC_CODE_PAGES * PAGE_SIZE, "process: user image too large");
    memcpy((void *) p->code_phys, user_image_start, image_size);

    mmu_map_section(p->pgd, PROC_STACK_VA_MB, p->stack_phys >> 20, false);
    mmu_map_section(p->pgd, PROC_CODE_VA_MB,  p->code_phys  >> 20, false);

    p->pid = next_pid++;
    p->sp  = PROC_STACK_TOP - 16; // 16-byte aligned, safety gap from top

    info("process: pid=%d code phys=0x%p va=0x%p image=%u bytes",
         p->pid, p->code_phys, PROC_CODE_VA, image_size);

    return p;
}

[[noreturn]]
void process_exit([[maybe_unused]] struct process *p, [[maybe_unused]] const int code) {
    info("process: pid=%d exited with code %d", p->pid, code);
    mm_page_free_n(p->code_phys,  PROC_CODE_PAGES);
    mm_page_free_n(p->stack_phys, PROC_STACK_PAGES);
    mmu_free_proc_table(p->pgd);
    kfree(p);
    poweroff();
}

void sched_tick(void) {}

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
                 : "r"(p->sp), "r"(sys_exit_stub), "r"(PROC_CODE_VA)
                 : "memory");

    __builtin_unreachable();
}
