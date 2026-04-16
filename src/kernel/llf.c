#include "kernel/llf.h"

#include "kernel/dev/memory.h"
#include "kernel/dev/mmu.h"
#include "kernel/logs.h"
#include "kernel/mem/alloc.h"
#include "libc/builtins.h"

#define L2_IDX(va) (((va) >> 12) & 0xFF)

// Get or create the L2 table for a given 1MB VA slot, attaching it to the L1 table.
static l2_entry *get_or_alloc_l2(l1_entry *pgd, u32 va_mb) {
    if (pgd[va_mb].type == L1_PAGE_TABLE)
        return phys_to_virt((uptr) pgd[va_mb].page_table.address << 10);
    l2_entry *pt = mmu_alloc_l2_table();
    if (pt) mmu_attach_l2(pgd, va_mb, pt);
    return pt;
}

// Allocate and map pages covering [va, va+memsz) into pgd.
// va need not be page-aligned; already-mapped pages (e.g. code/BSS overlap) are skipped.
static bool map_pages(l1_entry *pgd, u32 va, u32 memsz) {
    const u32 page_base = va & ~(PAGE_SIZE - 1u);
    const u32 pages     = (va - page_base + memsz + PAGE_SIZE - 1) / PAGE_SIZE;
    for (u32 i = 0; i < pages; i++) {
        u32       page_va = page_base + i * PAGE_SIZE;
        l2_entry *pt      = get_or_alloc_l2(pgd, page_va >> 20);
        if (!pt) return false;
        if (pt[L2_IDX(page_va)].type != L2_INVALID) continue; // already mapped
        uptr pa = mm_page_alloc();
        if (!pa) return false;
        mmu_map_page(pt, page_va, pa);
    }
    return true;
}

bool llf_load(l1_entry *pgd, const void *buf, u32 buf_size, uptr *out_entry) {
    if (buf_size < sizeof(llf_header_t)) {
        err("llf: buffer too small");
        return false;
    }

    const llf_header_t *hdr = buf;
    if (hdr->magic != LLF_MAGIC) {
        err("llf: bad magic 0x%x", hdr->magic);
        return false;
    }

    const llf_phdr_t *phdrs = (const llf_phdr_t *) ((const u8 *) buf + hdr->ph_off);

    for (u32 i = 0; i < hdr->ph_count; i++) {
        const llf_phdr_t ph = phdrs[i];
        if (ph.type == LLF_SEG_NULL) continue;

        if (!map_pages(pgd, ph.vaddr, ph.memsz)) {
            err("llf: OOM mapping segment %u (va=0x%x, memsz=%u)", i, ph.vaddr, ph.memsz);
            return false;
        }
    }

    // Switch TTBR0 to the process table so we can write to user VAs.
    mmu_set_proc_table(pgd);

    for (u32 i = 0; i < hdr->ph_count; i++) {
        const llf_phdr_t ph = phdrs[i];
        if (ph.type == LLF_SEG_NULL) continue;

        memset((void *) ph.vaddr, ph.memsz, 0);
        if (ph.type == LLF_SEG_LOAD && ph.filesz > 0)
            memcpy((void *) ph.vaddr, (const u8 *) buf + ph.offset, ph.filesz);
    }

    // Clean D-cache and invalidate I-cache so freshly written code is visible
    // to instruction fetches. Without this, a reused physical page still has
    // the previous process's code in the I-cache.
    asm volatile("mcr p15, 0, %0, c7, c10, 0 \n\t" // DCCIMVAC — clean+invalidate D-cache
                 "mcr p15, 0, %0, c7, c5,  0 \n\t" // ICIALLU  — invalidate entire I-cache
                 "dsb                          \n\t"
                 "isb" ::"r"(0)
                 : "memory");

    // Restore TTBR0 to the kernel table; process_exec will reinstall the process table.
    mmu_set_proc_table((l1_entry *) kernel_translation_table);

    *out_entry = hdr->entry;
    return true;
}