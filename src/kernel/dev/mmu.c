#include "kernel/dev/mmu.h"
#include "kernel/cpu.h"
#include "kernel/linker.h"
#include "kernel/dev/memory.h"

#define MB_SHIFT  20
#define MAX_PROCS 4

[[gnu::section(".tt"), gnu::aligned(0x4000)]]
translation_table kernel_translation_table;

[[gnu::section(".tt"), gnu::aligned(0x400)]]
page_table devices_page_table;

[[gnu::section(".tt"), gnu::aligned(0x4000)]]
static translation_table proc_tables[MAX_PROCS];
static bool              proc_table_used[MAX_PROCS];

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

    // Kernel High-Half Mapping
    table[KERNEL_VA >> MB_SHIFT] = (l1_entry) {.section = {
                                                   .type = L1_SECTION,
                                                   .address = kphys >> MB_SHIFT,
                                                   .ap_low = 0b10,
                                                   .type_ext = 0b001,
                                                   .bufferable = true,
                                                   .cacheable = true,
                                               }};
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
    asm volatile("mcr p15, 0, %0, c8, c7, 0\n\t" // invalidate TLB
                 "dsb\n\tisb" ::"r"(0));
}

translation_table *mmu_alloc_proc_table(void) {
    for (u32 i = 0; i < MAX_PROCS; i++) {
        if (!proc_table_used[i]) {
            proc_table_used[i] = true;
            // clone kernel mappings into process table
            for (u32 j = 0; j < 0x1000; j++)
                proc_tables[i][j] = kernel_translation_table[j];
            return &proc_tables[i];
        }
    }
    return nullptr;
}

void mmu_free_proc_table(translation_table *tt) {
    for (u32 i = 0; i < MAX_PROCS; i++) {
        if (&proc_tables[i] == tt) {
            proc_table_used[i] = false;
            return;
        }
    }
}

void mmu_map_section(translation_table *tt, u32 va_mb, u32 pa_mb, bool device) {
    (*tt)[va_mb] = (l1_entry) {.section = {
                                   .type = L1_SECTION,
                                   .address = pa_mb,
                                   .ap_low = 0b11, // RW kernel+user
                                   .type_ext = device ? 0b000 : 0b001,
                                   .bufferable = true,
                                   .cacheable = !device,
                               }};
}

void mmu_set_proc_table(translation_table *tt) {
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
    asm("mcr p15, 0, %0, c2, c0, 0;" // TTBR0
        "mcr p15, 0, %0, c2, c0, 1"  // TTBR1
        ::"r"((void *) table));

    asm("mcr p15, 0, %0, c3, c0, 0" ::"r"(1)); // Domain Access Control Register
    asm("mcr p15, 0, %0, c2, c0, 2" ::"r"(0)); // TTBCR = 0 (use TTBR0 for all)

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
