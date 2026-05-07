//
// Created by Romir Kulshrestha on 07/05/2026.
//

#include "kernel/pgd.h"

#include "kernel/dev/memory.h"
#include "kernel/mem/alloc.h"
#include "libc/builtins.h"

#define L2_IDX(va) (((va) >> 12) & 0xFF)

// Allocate a zeroed L2 page table (256 × 4KB small-page entries, 1KB-aligned).
static l2_entry *l2_new(void) {
    l2_entry *pt = kmalloc_aligned(sizeof(page_table), 0x400); // 1KB-aligned
    if (pt) memset(pt, 0, sizeof(page_table));
    return pt;
}

// Install an L2 table into the L1 entry for va_mb.
static void pgd_attach_l2(l1_entry *tt, u32 va_mb, l2_entry *pt) {
    u32 phys  = virt_to_phys(pt);
    tt[va_mb] = (l1_entry) {.page_table = {
                                .type    = L1_PAGE_TABLE,
                                .address = phys >> 10,
                                .domain  = 0,
                            }};
}

// Get or create the L2 table for a given 1MB VA slot.
static l2_entry *get_or_alloc_l2(l1_entry *pgd, const u32 va_mb) {
    if (pgd[va_mb].type == L1_PAGE_TABLE)
        return phys_to_virt((uptr) pgd[va_mb].page_table.address << 10);
    l2_entry *l2_table = l2_new();
    if (l2_table) pgd_attach_l2(pgd, va_mb, l2_table);
    return l2_table;
}

// Map one 4KB page: va and pa must be PAGE_SIZE-aligned.
static void l2_map_page(l2_entry *l2_table, u32 va, u32 pa) {
    l2_table[L2_IDX(va)] = (l2_entry) {.small_page = {
                                           .type       = L2_SMALL_PAGE,
                                           .address    = pa >> 12,
                                           .ap_low     = 0b11, // RW kernel+user
                                           .type_ext   = 0b001,
                                           .bufferable = true,
                                           .cacheable  = true,
                                       }};
}

pgd_t *pgd_new(void) {
    l1_entry *tt = kmalloc_aligned(PROC_TABLE_SIZE, PROC_TABLE_ALIGN);
    if (tt) memset(tt, 0, PROC_TABLE_SIZE);
    return tt;
}

pgd_t *pgd_clone(pgd_t *pgd) {
    auto new_pgd = pgd_new();
    if (!new_pgd) return nullptr;

    // for every megabyte in user space
    for (u32 mb = 0; mb < PROC_VA_MB; ++mb) {
        if (pgd[mb].type != L1_PAGE_TABLE) continue;
        // if a table exists to map it
        // get the existing table
        l2_entry *src_l2_table = phys_to_virt(pgd[mb].page_table.address << 10);
        // create the new table
        l2_entry *dst_l2_table = l2_new();
        if (!dst_l2_table)
            // OOM on L2 alloc
            goto err;

        // attach to L1
        pgd_attach_l2(new_pgd, mb, dst_l2_table);

        // for every possible L2 page
        for (int i = 0; i < 256; ++i) {
            if (src_l2_table[i].type != L2_SMALL_PAGE) continue;
            // if a page exists

            uptr src_pa = (uptr) src_l2_table[i].small_page.address << PAGE_SHIFT;

            // allocate a page
            uptr dst_pa = mm_page_alloc();
            if (!dst_pa)
                // OOM on page alloc
                goto err;

            // copy the page
            memcpy(phys_to_virt(dst_pa), phys_to_virt(src_pa), PAGE_SIZE);

            // attach to L2
            l2_map_page(dst_l2_table, (mb << MB_SHIFT) | (i << PAGE_SHIFT), dst_pa);
        }
    }
    return new_pgd;

err:
    pgd_free(new_pgd);
    return nullptr;
}

void *pgd_map_user_page(pgd_t *pgd, void *va) {
    const uptr va_mb = (uptr) va >> MB_SHIFT;

    // get L2 table
    l2_entry *l2_table = get_or_alloc_l2(pgd, va_mb);
    if (!l2_table) return nullptr;

    // allocate a new physical page
    const uptr page_pa = mm_page_alloc();
    if (!page_pa) return nullptr;

    l2_map_page(l2_table, (uptr) va, page_pa);
    return phys_to_virt(page_pa);
}

void pgd_free(pgd_t *pgd) {
    if (!pgd) return;
    // for every megabyte in user space
    for (u32 mb = 0; mb < PROC_VA_MB; ++mb) {
        if (pgd[mb].type != L1_PAGE_TABLE) continue;
        // if a table exists to map it
        // get the table
        l2_entry *l2_table = phys_to_virt(pgd[mb].page_table.address << 10);
        // for every possible table entry
        for (int i = 0; i < 256; ++i) {
            // if an entry exists
            if (l2_table[i].type == L2_SMALL_PAGE)
                // free it
                mm_page_free(l2_table[i].small_page.address << PAGE_SHIFT);
        }
        // free the table
        kfree(l2_table);
    }
    // free the whole PGD
    kfree(pgd);
}
