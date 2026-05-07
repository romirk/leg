#include "kernel/llf.h"

#include "kernel/dev/mmu.h"
#include "kernel/logs.h"
#include "kernel/mem/alloc.h"
#include "kernel/pgd.h"
#include "libc/builtins.h"

// Allocate and map pages covering [va, va+memsz) into pgd.
// va need not be page-aligned; already-mapped pages (e.g. code/BSS overlap) are skipped.
static bool map_pages(l1_entry *pgd, uptr va, uptr memsz) {
    const uptr page_base = va & ~(PAGE_SIZE - 1u);
    const uptr pages     = (va - page_base + memsz + PAGE_SIZE - 1) / PAGE_SIZE;
    for (u32 i = 0; i < pages; i++) {
        pgd_map_user_page(pgd, (void *) page_base + i * PAGE_SIZE);
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

        memclr((void *) ph.vaddr, ph.memsz);
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