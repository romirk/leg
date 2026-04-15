#include "kernel/dev/mmu.h"

#include "kernel/cpu.h"
#include "kernel/dev/memory.h"
#include "kernel/linker.h"
#include "kernel/mem/alloc.h"
#include "kernel/process.h"
#include "libc/builtins.h"

#define MB_SHIFT 20

[[gnu::section(".tt"), gnu::aligned(0x4000)]]
translation_table kernel_translation_table;

[[gnu::section(".tt"), gnu::aligned(0x400)]]
page_table devices_page_table;

// offset of a kernel virtual symbol within the kernel image
#define KRN_OFFSET(sym) ((u32) (sym) - (u32) kernel_main_beg)
#define L2_IDX(virt)    (((virt) >> 12) & 0xFF)
// Helper macro for L2 Small Page entries
#define MAKE_L2_DEVICE(phys)                                                                       \
    (l2_entry) {                                                                                   \
        .small_page = {                                                                            \
            .type = L2_SMALL_PAGE,                                                                 \
            .address = (phys) >> 12,                                                               \
            .ap_low = 0b10,                                                                        \
            .type_ext = 0b000,                                                                     \
            .bufferable = true,                                                                    \
            .cacheable = false,                                                                    \
        }                                                                                          \
    }

[[gnu::section(".startup.mmu")]]
static void map_sections(void *dtb) {
    const u32 kphys = ((u32) dtb & ~0xFFFFF) + KPHYS_OFFSET;

    const auto table = (l1_entry *) (kphys + KRN_OFFSET(kernel_translation_table));
    const auto device_table = (l2_entry *) (kphys + KRN_OFFSET(devices_page_table));
    const uptr devices_pt_phys = (uptr) &devices_page_table - KERNEL_VA + kphys;

    table[DEVICE_VIRT_BLOCK >> MB_SHIFT] =
        (l1_entry) {.page_table = {
                        .type = L1_PAGE_TABLE,
                        .address = devices_pt_phys >> 10, // Base address bits [31:10]
                        .domain = 0,
                    }};

    l2_entry device_entry_template = {.small_page = {
                                          .type = L2_XN_PAGE, // 0b11: Small Page + Execute Never
                                          .bufferable = true,
                                          .cacheable = false,
                                          .ap_low = 0b10, // Privileged Read/Write
                                      }};

    // UART
    device_entry_template.small_page.address = UART0_PHYSICAL >> 12;
    device_table[L2_IDX(UART0_BASE)] = device_entry_template;

    // GIC Distributor
    device_entry_template.small_page.address = GICD_PHYSICAL >> 12;
    device_table[L2_IDX(GICD_BASE)] = device_entry_template;

    // GIC CPU Interface
    device_entry_template.small_page.address = GICC_PHYSICAL >> 12;
    device_table[L2_IDX(GICC_BASE)] = device_entry_template;

    // RTC
    device_entry_template.small_page.address = RTC_PHYSICAL >> 12;
    device_table[L2_IDX(RTC_BASE)] = device_entry_template;

    // FWCFG
    device_entry_template.small_page.address = FWCFG_PHYSICAL >> 12;
    device_table[L2_IDX(FWCFG_BASE)] = device_entry_template;

    // Map 16KB of VirtIO MMIO space (4 consecutive 4KB pages)
    for (int i = 0; i < 4; i++) {
        uptr v_addr = VIRTIO_MMIO_BASE + (i * 0x1000);
        uptr p_addr = VIRTIO_MMIO_PHYSICAL + (i * 0x1000);

        device_entry_template.small_page.address = p_addr >> 12;
        device_table[L2_IDX(v_addr)] = device_entry_template;
    }

    // Identity map ROM (first 1MB)
    table[0x000] = (l1_entry) {.section = {
                                   .type = L1_SECTION,
                                   .address = 0x000,
                                   .ap_low = 0b10,
                                   .type_ext = 0b001,
                                   .bufferable = true,
                                   .cacheable = true,
                               }};

    // Kernel High-Half Mapping — 4MB covers code, BSS, and heap
    for (u32 i = 0; i < 4; i++) {
        table[(KERNEL_VA >> MB_SHIFT) + i] = (l1_entry) {.section = {
                                                             .type = L1_SECTION,
                                                             .address = (kphys >> MB_SHIFT) + i,
                                                             .ap_low = 0b10,
                                                             .type_ext = 0b001,
                                                             .bufferable = true,
                                                             .cacheable = true,
                                                         }};
    }
    // DTB / early RAM — identity-map 4MB from DTB base
    const u32 dtb_section = (u32) dtb >> MB_SHIFT;
    for (u32 i = 0; i < 4; ++i) {
        table[dtb_section + i] = (l1_entry) {.section = {
                                                 .type = L1_SECTION,
                                                 .address = dtb_section + i,
                                                 .ap_low = 0b10,
                                                 .type_ext = 0b001,
                                                 .bufferable = true,
                                                 .cacheable = true,
                                             }};
    }
}

void mmu_map_identity(u32 phys_mb, bool device) {
    kernel_translation_table[phys_mb] = (l1_entry) {.section = {
                                                        .type = L1_SECTION,
                                                        .address = phys_mb,
                                                        .ap_low = 0b10,
                                                        .type_ext = device ? 0b000 : 0b001,
                                                        .bufferable = true,
                                                        .cacheable = !device,
                                                    }};
    const uptr entry_va = (uptr) &kernel_translation_table[phys_mb];
    asm volatile("mcr p15, 0, %0, c7, c10, 1\n\t" // DCCMVAC: clean D-cache entry to PoC
                 "dsb\n\t"
                 "mcr p15, 0, %0, c8, c7,  0\n\t" // TLBIALL: invalidate unified TLB
                 "dsb\n\tisb" ::"r"(entry_va));
}

l1_entry *mmu_alloc_proc_table(void) {
    l1_entry *tt = kmalloc_aligned(PROC_TABLE_SIZE, PROC_TABLE_ALIGN);
    if (tt) memset(tt, 0, PROC_TABLE_SIZE);
    return tt;
}

void mmu_free_proc_table(l1_entry *tt) {
    kfree(tt);
}

void mmu_map_section(l1_entry *tt, u32 va_mb, u32 pa_mb, bool device) {
    tt[va_mb] = (l1_entry) {.section = {
                                .type = L1_SECTION,
                                .address = pa_mb,
                                .ap_low = 0b11, // RW kernel+user
                                .type_ext = device ? 0b000 : 0b001,
                                .bufferable = true,
                                .cacheable = !device,
                            }};
}

l2_entry *mmu_alloc_l2_table(void) {
    l2_entry *pt = kmalloc_aligned(sizeof(page_table), 0x400); // 1KB-aligned
    if (pt) memset(pt, 0, sizeof(page_table));
    return pt;
}

void mmu_free_l2_table(l2_entry *pt) {
    kfree(pt);
}

void mmu_attach_l2(l1_entry *tt, u32 va_mb, l2_entry *pt) {
    u32 phys = virt_to_phys(pt);
    tt[va_mb] = (l1_entry) {.page_table = {
                                .type = L1_PAGE_TABLE,
                                .address = phys >> 10,
                                .domain = 0,
                            }};
}

void mmu_map_page(l2_entry *pt, u32 va, u32 pa) {
    pt[L2_IDX(va)] = (l2_entry) {.small_page = {
                                     .type = L2_SMALL_PAGE,
                                     .address = pa >> 12,
                                     .ap_low = 0b11, // RW kernel+user
                                     .type_ext = 0b001,
                                     .bufferable = true,
                                     .cacheable = true,
                                 }};
}

void mmu_set_proc_table(l1_entry *tt) {
    u32 phys = virt_to_phys(tt);
    asm volatile("mcr p15, 0, %0, c2, c0, 0\n\t" // TTBR0 = process table
                 "mcr p15, 0, %0, c8, c7, 0\n\t" // invalidate entire TLB
                 "dsb\n\t"
                 "isb" ::"r"(phys)
                 : "memory");
}

[[gnu::section(".startup.mmu")]]
void init_mmu(void *dtb) {
    map_sections(dtb);

    const u32 kphys = ((u32) dtb & ~0xFFFFF) + KPHYS_OFFSET;
    auto      table = (l1_entry *) (kphys + KRN_OFFSET(kernel_translation_table));

    // TTBCR.N=7: VA[31:25]==0 → TTBR0 (user [0, 0x02000000)); else → TTBR1 (kernel).
    // Must be written before TTBR0/TTBR1 to establish the split before MMU enable.
    asm("mcr p15, 0, %0, c2, c0, 2" ::"r"(TTBCR_N)); // TTBCR
    asm("mcr p15, 0, %0, c2, c0, 1" ::"r"(table));   // TTBR1 = kernel table (16KB-aligned)
    asm("mcr p15, 0, %0, c2, c0, 0" ::"r"(table));   // TTBR0 = kernel table until first process

    asm("mcr p15, 0, %0, c3, c0, 0" ::"r"(1)); // Domain Access Control Register

    // flush caches and TLB before enabling MMU
    asm("mcr p15, 0, %0, c7,  c7, 0;" // invalidate I+D caches
        "mcr p15, 0, %0, c8,  c7, 0;" // invalidate TLB
        "dsb;"
        "isb;" ::"r"(0));

    // enable MMU
    struct sctlr sctlr;
    asm("mrc p15, 0, %0, c1, c0, 0" : "=r"(sctlr));
    sctlr.M = true;
    asm("mcr p15, 0, %0, c1, c0, 0" ::"r"(sctlr) : "memory");

    // we are now in virtual address space
}
