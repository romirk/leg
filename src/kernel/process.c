// process.c — process creation and execution

#include "kernel/process.h"

#include "kernel/arch_proc.h"
#include "kernel/dev/mmu.h"
#include "kernel/fs.h"
#include "kernel/llf.h"
#include "kernel/logs.h"
#include "kernel/mem/alloc.h"
#include "kernel/pgd.h"
#include "kernel/scheduler.h"
#include "libc/cstring.h"
#include "utils.h"

static pid_t next_pid = 1;

// Walk the process L1 table and free every mapped page and L2 table.
static void process_free(struct process *p) {
    pgd_free(p->pgd);
    kfree(p);
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

    p->pgd = pgd_new();
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
    arch_ctx_init_fresh(&p->ctx, p->entry, p->sp);

    info("process: pid=%d entry=0x%x stack_top=0x%x (%u stack pages)", p->pid, p->entry,
         PROC_STACK_TOP, p->stack_pages);

    return p;
}

bool process_add_page(struct process *p) {
    const uptr va = PROC_STACK_TOP - (p->stack_pages + 1) * PAGE_SIZE;
    if (!pgd_map_user_page(p->pgd, (void *) va)) {
        err("process: OOM growing stack (page %u)", p->stack_pages);
        return false;
    }
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

process_t *process_fork(const uptr lr_svc, const uptr sp_usr, const uptr state) {
    process_t *parent = (process_t *) current_proc;

    process_t *child = kmalloc_aligned(sizeof(*child), 0x4000);
    if (!child) {
        err("fork: OOM for struct");
        return nullptr;
    }

    // Deep-copy all mapped user pages
    child->pgd = pgd_clone(parent->pgd);
    if (!child->pgd) {
        err("fork: OOM for child PGD");
        process_free(child);
        return nullptr;
    }

    // Clone context; child returns 0 from fork, resumes at the SVC return address
    arch_ctx_init_fork(&child->ctx, &parent->ctx, lr_svc, sp_usr, state);

    child->pid         = next_pid++;
    child->entry       = parent->entry;
    child->sp          = sp_usr;
    child->heap_end    = parent->heap_end;
    child->stack_pages = parent->stack_pages;
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
    current_proc = p;
    sched_add(p);
    arch_eret_to_user(&p->ctx);
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
    l1_entry *new_pgd = pgd_new();
    if (!new_pgd) {
        err("process_replace: OOM for pgd");
        return;
    }

    uptr entry = 0;
    if (!load_user_image(new_pgd, kname, &entry)) {
        err("process_replace: failed to load '%s'", kname);
        pgd_free(new_pgd);
        return;
    }

    // Map initial stack pages into the new address space using a temporary
    // process_t shell so process_add_page can drive the allocation loop.
    process_t tmp = {.pgd = new_pgd, .stack_pages = 0};
    if (!setup_initial_stack(&tmp)) {
        err("process_replace: OOM for stack");
        pgd_free(new_pgd);
        return;
    }

    // Swap in the new address space, freeing the old one.
    l1_entry *old_pgd = p->pgd;
    p->pgd            = new_pgd;
    p->entry          = entry;
    p->sp             = PROC_STACK_TOP - 16;
    p->heap_end       = PROC_HEAP_START;
    p->stack_pages    = tmp.stack_pages;

    // Set ctx for a clean entry: argc=0, argv=null, fresh sp.
    arch_ctx_init_fresh(&p->ctx, entry, p->sp);

    info("process_replace: pid=%d → '%s' entry=%p", pid, kname, (void *) entry);

    // Free old address space AFTER installing the new one.
    pgd_free(old_pgd);

    if (p == current_proc) {
        context_switch(p);
        __builtin_unreachable();
    }
}