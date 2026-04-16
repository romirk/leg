// process.c — process creation and execution

#include "kernel/process.h"

#include "kernel/cpu.h"
#include "kernel/dev/memory.h"
#include "kernel/dev/mmu.h"
#include "kernel/exceptions.h"
#include "kernel/fs.h"
#include "kernel/llf.h"
#include "kernel/logs.h"
#include "kernel/mem/alloc.h"
#include "kernel/scheduler.h"
#include "libc/builtins.h"
#include "libc/cstring.h"
#include "utils.h"

static pid_t next_pid = 1;

// Walk the process L1 table and free every mapped page and L2 table.
static void process_free(struct process *p) {
    for (u32 mb = 0; mb < PROC_VA_MB; mb++) {
        if (p->pgd[mb].type != L1_PAGE_TABLE) continue;
        l2_entry *pt = phys_to_virt((uptr) p->pgd[mb].page_table.address << 10);
        for (u32 i = 0; i < 256; i++) {
            if (pt[i].type == L2_SMALL_PAGE)
                mm_page_free((uptr) pt[i].small_page.address << PAGE_SHIFT);
        }
        mmu_free_l2_table(pt);
    }
    mmu_free_proc_table(p->pgd);
    kfree(p);
}

// Get or create the L2 table for a given 1MB VA slot.
static l2_entry *get_or_alloc_l2(l1_entry *pgd, u32 va_mb) {
    if (pgd[va_mb].type == L1_PAGE_TABLE)
        return phys_to_virt((uptr) pgd[va_mb].page_table.address << 10);
    l2_entry *pt = mmu_alloc_l2_table();
    if (pt) mmu_attach_l2(pgd, va_mb, pt);
    return pt;
}

// Load an executable from the filesystem into pgd
static bool load_user_image(l1_entry *pgd, const char *name, uptr *out_entry) {
    const fs_blob_t *blob = fs_find(name);
    if (!blob || blob->flags != FS_TYPE_EXECUTABLE) {
        err("load_user_image: executable '%s' not found", name);
        return false;
    }
    u32   llf_size = align_up(blob->size, 512);
    void *buf      = kmalloc(llf_size);
    if (!buf) {
        err("load_user_image: OOM for LLF buffer");
        return false;
    }
    bool ok = fs_read(blob, buf) && llf_load(pgd, buf, llf_size, out_entry);
    kfree(buf);
    return ok;
}

// Allocate PROC_STACK_INIT_PAGES stack pages into p->pgd
// sets p->stack_pt and p->stack_pages
static bool setup_initial_stack(struct process *p) {
    p->stack_pt = get_or_alloc_l2(p->pgd, PROC_STACK_TOP >> 20);
    if (!p->stack_pt) {
        err("setup_initial_stack: OOM for stack L2 table");
        return false;
    }
    p->stack_pages = 0;
    for (u32 i = 0; i < PROC_STACK_INIT_PAGES; i++) {
        if (!process_add_page(p)) {
            err("setup_initial_stack: OOM for stack page %u", i);
            return false;
        }
    }
    return true;
}

struct process *process_create(const char *name) {
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

    if (!load_user_image(p->pgd, name, &p->entry)) {
        process_free(p);
        return nullptr;
    }

    if (!setup_initial_stack(p)) {
        process_free(p);
        return nullptr;
    }

    p->pid      = next_pid++;
    p->sp       = PROC_STACK_TOP - 16;
    p->heap_end = PROC_HEAP_START;

    info("process: pid=%d entry=0x%x stack_top=0x%x (%u stack pages)", p->pid, p->entry,
         PROC_STACK_TOP, p->stack_pages);

    return p;
}

bool process_add_page(struct process *p) {
    const u32 va    = PROC_STACK_TOP - (p->stack_pages + 1) * PAGE_SIZE;
    const u32 va_mb = va >> 20;

    // If the stack has crossed into a new 1MB slot, attach a fresh L2 table.
    if (p->stack_pages > 0 && va_mb != (PROC_STACK_TOP - p->stack_pages * PAGE_SIZE) >> 20) {
        l2_entry *pt = mmu_alloc_l2_table();
        if (!pt) {
            err("process: OOM for stack L2 table");
            return false;
        }
        mmu_attach_l2(p->pgd, va_mb, pt);
        p->stack_pt = pt;
    }

    uptr pa = mm_page_alloc();
    if (!pa) {
        err("process: OOM growing stack (page %u)", p->stack_pages);
        return false;
    }

    mmu_map_page(p->stack_pt, va, pa);
    p->stack_pages++;
    return true;
}

[[noreturn]]
void process_exit(pid_t pid, const int code) {
    process_t *p = sched_get(pid);
    if (!p) {
        err("process_exit: no process with pid %d", pid);
    }

    int was_current = (p == current_proc);
    sched_remove(pid);
    process_free(p);

    info("process: pid=%d exited with code %d", pid, code);

    sched_wake_joiners(pid, code);

    if (was_current) {
        auto next = sched_pick_next();
        if (!next) {
            warn("process_exit: no runnable processes remain; halting");
            poweroff();
        }
        context_switch(next);
        __builtin_unreachable();
    }
    limbo;
}

process_t *process_fork(u32 lr_svc, u32 sp_usr, u32 cpsr) {
    process_t *parent = (process_t *) current_proc;

    process_t *child = kmalloc_aligned(sizeof(*child), 0x4000);
    if (!child) {
        err("fork: OOM for struct");
        return nullptr;
    }

    child->pgd = mmu_alloc_proc_table();
    if (!child->pgd) {
        err("fork: OOM for L1 table");
        kfree(child);
        return nullptr;
    }

    // Deep-copy all mapped user pages
    for (u32 mb = 0; mb < PROC_VA_MB; mb++) {
        if (parent->pgd[mb].type != L1_PAGE_TABLE) continue;
        l2_entry *src_pt = phys_to_virt((uptr) parent->pgd[mb].page_table.address << 10);
        l2_entry *dst_pt = mmu_alloc_l2_table();
        if (!dst_pt) {
            err("fork: OOM for L2 table");
            process_free(child);
            return nullptr;
        }
        mmu_attach_l2(child->pgd, mb, dst_pt);
        for (u32 i = 0; i < 256; i++) {
            if (src_pt[i].type != L2_SMALL_PAGE) continue;
            uptr src_pa = (uptr) src_pt[i].small_page.address << PAGE_SHIFT;
            uptr dst_pa = mm_page_alloc();
            if (!dst_pa) {
                err("fork: OOM for page");
                process_free(child);
                return nullptr;
            }
            memcpy(phys_to_virt(dst_pa), phys_to_virt(src_pa), PAGE_SIZE);
            mmu_map_page(dst_pt, (mb << 20) | (i << PAGE_SHIFT), dst_pa);
        }
    }

    // Clone context; child returns 0 from fork, resumes at the SVC return address
    child->ctx      = parent->ctx;
    child->ctx.r[0] = 0;      // child's sys_fork() returns 0
    child->ctx.pc   = lr_svc; // resume at instruction after svc
    child->ctx.sp   = sp_usr;
    child->ctx.cpsr = cpsr;

    child->pid         = next_pid++;
    child->entry       = parent->entry;
    child->sp          = sp_usr;
    child->heap_end    = parent->heap_end;
    child->stack_pages = parent->stack_pages;
    child->stack_pt    = nullptr; // not used post-fork
    child->wake_tick   = 0;
    child->suspended   = 0;

    if (sched_add(child) < 0) {
        process_free(child);
        return nullptr;
    }

    info("fork: parent pid=%d → child pid=%d", parent->pid, child->pid);
    return child;
}

[[noreturn]]
void process_exec(struct process *p) {
    mmu_set_proc_table(p->pgd);

    psr_t s = {
        .M = usr,
        .I = false,
    };
    write_spsr(s);

    current_proc = p;
    sched_add(p);

    asm volatile("cps #31          \n\t" // System Mode
                 "mov sp, %0       \n\t" // Set sp_usr
                 "cps #19          \n\t" // Back to SVC Mode
                 "mov lr, %1       \n\t" // entry point
                 "movs pc, lr      \n\t" // Mode switch
                 :
                 : "r"(p->sp), "r"(p->entry)
                 : "memory");

    __builtin_unreachable();
}

void process_replace(pid_t pid, char *name) {
    process_t *p = sched_get(pid);
    if (!p) {
        err("process_replace: pid %d not found", pid);
        return;
    }

    // name is a user-space pointer — copy it before the page tables die a horrible death.
    char kname[64];
    strcpy(kname, name);

    // Load the new binary into a fresh address space.
    l1_entry *new_pgd = mmu_alloc_proc_table();
    if (!new_pgd) {
        err("process_replace: OOM for pgd");
        return;
    }

    uptr entry = 0;
    if (!load_user_image(new_pgd, kname, &entry)) {
        err("process_replace: failed to load '%s'", kname);
        mmu_free_proc_table(new_pgd);
        return;
    }

    // Map initial stack pages into the new address space using a temporary
    // process_t shell so process_add_page can drive the allocation loop.
    process_t tmp = {.pgd = new_pgd, .stack_pt = nullptr, .stack_pages = 0};
    if (!setup_initial_stack(&tmp)) {
        err("process_replace: OOM for stack");
        // Free pages already mapped into new_pgd before bailing.
        for (u32 mb = 0; mb < PROC_VA_MB; mb++) {
            if (new_pgd[mb].type != L1_PAGE_TABLE) continue;
            l2_entry *pt = phys_to_virt((uptr) new_pgd[mb].page_table.address << 10);
            for (u32 i = 0; i < 256; i++) {
                if (pt[i].type == L2_SMALL_PAGE)
                    mm_page_free((uptr) pt[i].small_page.address << PAGE_SHIFT);
            }
            mmu_free_l2_table(pt);
        }
        mmu_free_proc_table(new_pgd);
        return;
    }

    // Swap in the new address space, freeing the old one.
    l1_entry *old_pgd = p->pgd;
    p->pgd            = new_pgd;
    p->entry          = entry;
    p->sp             = PROC_STACK_TOP - 16;
    p->heap_end       = PROC_HEAP_START;
    p->stack_pt       = tmp.stack_pt;
    p->stack_pages    = tmp.stack_pages;

    // Set ctx for a clean entry: argc=0, argv=null, fresh sp.
    memset(&p->ctx, sizeof(p->ctx), 0);
    p->ctx.sp   = p->sp;
    p->ctx.pc   = entry;
    p->ctx.cpsr = 0x10u; // USR mode, IRQs enabled

    info("process_replace: pid=%d → '%s' entry=%p", pid, kname, (void *) entry);

    // Free old address space AFTER installing the new one.
    for (u32 mb = 0; mb < PROC_VA_MB; mb++) {
        if (old_pgd[mb].type != L1_PAGE_TABLE) continue;
        l2_entry *pt = phys_to_virt((uptr) old_pgd[mb].page_table.address << 10);
        for (u32 i = 0; i < 256; i++) {
            if (pt[i].type == L2_SMALL_PAGE)
                mm_page_free((uptr) pt[i].small_page.address << PAGE_SHIFT);
        }
        mmu_free_l2_table(pt);
    }
    mmu_free_proc_table(old_pgd);

    if (p == current_proc) {
        context_switch(p);
        __builtin_unreachable();
    }
}