// mmu.h — ARMv7 MMU and L1 translation table management

#ifndef MEMORY_H
#define MEMORY_H
#include "types.h"

#define KERNEL_VA 0xC0000000

// Set during early boot by kboot; physical base where kernel image lives in RAM.
extern u32 kernel_phys_base;

// L1 entry type bits [1:0]
typedef enum {
    L1_INVALID = 0b00,
    L1_PAGE_TABLE = 0b01, // points to an L2 page table
    L1_SECTION = 0b10,    // maps a 1MB section directly
    L1_PXN_SECTION = 0b11 // 1MB section with PXN set
} l1_descriptor;

// L2 entry type bits [1:0]
typedef enum {
    L2_INVALID = 0b00,
    L2_LARGE_PAGE = 0b01, // 64KB page
    L2_SMALL_PAGE = 0b10, // 4KB page
    L2_XN_PAGE = 0b11     // 4KB page with XN set
} l2_descriptor;

// One 32-bit L1 translation table entry.
// Interpret via the .section / .page_table / .supersection view depending on type.
typedef union {
    u32           raw;
    l1_descriptor type : 2; // peek at entry type without aliasing a sub-struct

    // type == L1_PAGE_TABLE — points to an L2 table
    struct [[gnu::packed]] [[gnu::aligned(4)]] {
        l1_descriptor type : 2;
        bool          privileged_execute_never : 1;
        bool          non_secure : 1;
        bool : 1;
        u8 domain : 4;
        bool : 1;
        u32 address : 22; // L2 table base address [31:10]
    } page_table;

    // type == L1_SECTION — maps a 1MB physical section
    struct [[gnu::packed]] [[gnu::aligned(4)]] {
        l1_descriptor type : 2;
        bool          bufferable : 1;
        bool          cacheable : 1;
        bool          execute_never : 1;
        u8            domain : 4;
        bool : 1;
        u8   access_perms : 2; // AP[1:0] — access permission bits
        u8   type_ext : 3;     // TEX[2:0] — memory type extension
        bool access_ext : 1;   // APX — makes AP read-only when set
        bool shareable : 1;
        bool not_global : 1;
        bool supersection : 1; // must be 0 for a regular section
        bool non_secure : 1;
        u32  address : 12; // section base address [31:20]
    } section;

    // type == L1_SECTION with supersection == 1 — maps a 16MB supersection
    struct [[gnu::packed]] [[gnu::aligned(4)]] {
        l1_descriptor type : 2;
        bool          bufferable : 1;
        bool          cacheable : 1;
        bool          execute_never : 1;
        u8            xaddr_39_36 : 4; // extended address bits [39:36]
        bool : 1;
        u8   access_perms : 2;
        u8   type_ext : 3;
        bool access_ext : 1;
        bool shareable : 1;
        bool not_global : 1;
        bool supersection : 1; // must be 1
        bool non_secure : 1;
        u8   xaddr_35_32 : 4; // extended address bits [35:32]
        u8   address;         // supersection base address [31:24]
    } supersection;
} l1_entry;

// One 32-bit L2 page table entry.
typedef union {
    u32           raw;
    l2_descriptor type : 2;

    // type == L2_LARGE_PAGE — maps a 64KB page
    struct [[gnu::packed]] [[gnu::aligned(4)]] {
        l2_descriptor type : 2;
        bool          bufferable : 1;
        bool          cacheable : 1;
        u8            access_perms : 2;
        u8 : 3;
        bool access_ext : 1;
        bool shareable : 1;
        bool not_global : 1;
        u8   type_ext : 3;
        bool execute_never : 1;
        u16  address; // large page base address [31:16]
    } large_page;

    // type == L2_SMALL_PAGE — maps a 4KB page
    struct [[gnu::packed]] [[gnu::aligned(4)]] {
        l2_descriptor type : 2;
        bool          bufferable : 1;
        bool          cacheable : 1;
        u8            access_perms : 2;
        u8            type_ext : 3;
        bool          access_ext : 1;
        bool          shareable : 1;
        bool          not_global : 1;
        u32           address : 20; // small page base address [31:12]
    } small_page;
} l2_entry;

// L1 translation table — 4096 entries, one per 1MB of virtual address space
typedef l1_entry translation_table[0x1000];
// L2 translation table
typedef l2_entry page_table[0x100];

// Configures the MMU and L1 translation table, ID-mapping flash, RAM, and UART address spaces.
void init_mmu(void *dtb);

// Map a 1MB physical section as identity (VA == PA). Flushes TLB.
void mmu_map_identity(u32 phys_mb, bool device);

// Convert a virtual address to physical.
// Kernel VA (>=0xC0000000) uses kernel_phys_base offset; otherwise identity.
static u32 virt_to_phys(const void *va) {
    u32 v = (u32) va;
    if (v >= KERNEL_VA) return kernel_phys_base + (v - KERNEL_VA);
    return v; // identity-mapped
}

extern translation_table kernel_translation_table;

// Allocate a process L1 table pre-cloned from the kernel table.
translation_table *mmu_alloc_proc_table(void);

void mmu_free_proc_table(translation_table *tt);

// Map a 1MB section at va_mb into a specific L1 table (not kernel_translation_table).
void mmu_map_section(translation_table *tt, u32 va_mb, u32 pa_mb, bool device);

// Switch TTBR0 to a process table (physical address). Flushes TLB.
void mmu_set_proc_table(translation_table *tt);

#endif // MEMORY_H
