// process.c — process creation and execution

#include "kernel/process.h"

#include "kernel/cpu.h"
#include "kernel/dev/memory.h"
#include "kernel/dev/mmu.h"
#include "kernel/fs.h"
#include "kernel/llf.h"
#include "kernel/logs.h"
#include "kernel/mem/alloc.h"
#include "utils.h"

static pid_t next_pid = 0;

struct process *current_proc = nullptr;

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

struct process *process_create(char *name) {
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

    // ── Load LLF from filesystem ─────────────────────────────────────────────
    const fs_blob_t *blob = fs_find(name);
    if (!blob || blob->flags != FS_TYPE_EXECUTABLE) {
        err("process: executable '%s' not found in filesystem", name);
        process_free(p);
        return nullptr;
    }

    u32   llf_size = align_up(blob->size, 512);
    void *llf_buf = kmalloc(llf_size);
    if (!llf_buf) {
        err("process: OOM for LLF buffer");
        process_free(p);
        return nullptr;
    }

    bool ok = fs_read(blob, llf_buf) && llf_load(p->pgd, llf_buf, llf_size, &p->entry);
    kfree(llf_buf);

    if (!ok) {
        process_free(p);
        return nullptr;
    }

    // ── Stack ─────────────────────────────────────────────────────────────────
    p->stack_pt = get_or_alloc_l2(p->pgd, PROC_STACK_TOP >> 20);
    if (!p->stack_pt) {
        err("process: OOM for stack L2 table");
        process_free(p);
        return nullptr;
    }
    p->stack_pages = 0;

    for (u32 i = 0; i < PROC_STACK_INIT_PAGES; i++) {
        if (!process_add_page(p)) {
            err("process: OOM for initial stack page %u", i);
            process_free(p);
            return nullptr;
        }
    }

    // ── Finalise ─────────────────────────────────────────────────────────────
    p->pid = next_pid++;
    p->sp = PROC_STACK_TOP - 16;
    p->heap_end = PROC_HEAP_START;

    info("process: pid=%d entry=0x%x stack_top=0x%x (%u stack pages)", p->pid, p->entry,
         PROC_STACK_TOP, p->stack_pages);

    return p;
}

bool process_add_page(struct process *p) {
    const u32 va = PROC_STACK_TOP - (p->stack_pages + 1) * PAGE_SIZE;
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
void process_exit(struct process *p, const int code) {
    info("process: pid=%d exited with code %d", p->pid, code);
    process_free(p);
    warn("kernel: shutdown");
    poweroff();
}

void sched_tick(void) {
}

[[noreturn]]
void process_exec(struct process *p) {
    current_proc = p;

    mmu_set_proc_table(p->pgd);

    psr_t s = {
        .M = usr,
        .I = false,
    };
    write_spsr(s);

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