// mmu.h — ARMv7 MMU and L1 translation table management

#ifndef MEMORY_H
#define MEMORY_H

#include "types.h"

#define MB_SHIFT 20

// L1 entry type bits [1:0]
typedef enum {
    L1_INVALID     = 0b00,
    L1_PAGE_TABLE  = 0b01, // points to an L2 page table
    L1_SECTION     = 0b10, // maps a 1MB section directly
    L1_PXN_SECTION = 0b11  // 1MB section with PXN set
} l1_descriptor;

// L2 entry type bits [1:0]
typedef enum {
    L2_INVALID    = 0b00,
    L2_LARGE_PAGE = 0b01, // 64KB page
    L2_SMALL_PAGE = 0b10, // 4KB page
    L2_XN_PAGE    = 0b11  // 4KB page with XN set
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
        u8   ap_low : 2;   // AP[1:0] — access permission bits
        u8   type_ext : 3; // TEX[2:0] — memory type extension
        bool ap_high : 1;  // APX — makes AP read-only when set
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
        u8   ap_low : 2;
        u8   type_ext : 3;
        bool ap_high : 1;
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
    struct [[gnu::packed, gnu::aligned(4)]] {
        l2_descriptor type : 2;
        bool          bufferable : 1;
        bool          cacheable : 1;
        u8            ap_low : 2;
        u8 : 3;
        bool ap_high : 1;
        bool shareable : 1;
        bool not_global : 1;
        u8   type_ext : 3;
        bool execute_never : 1;
        u16  address; // large page base address [31:16]
    } large_page;

    // type == L2_SMALL_PAGE — maps a 4KB page
    struct [[gnu::packed, gnu::aligned(4)]] {
        l2_descriptor type : 2;       // Bits [1:0]
        bool          bufferable : 1; // Bit 2
        bool          cacheable : 1;  // Bit 3
        u8            ap_low : 2;     // Bits [5:4]
        u8            type_ext : 3;   // Bits [8:6]
        bool          ap_high : 1;    // Bit 9
        bool          shareable : 1;  // Bit 10
        bool          not_global : 1; // Bit 11
        u32           address : 20;   // Bits [31:12]
    } small_page;
} l2_entry;

// L1 translation table — 4096 entries, one per 1MB of virtual address space
typedef l1_entry translation_table[0x1000];
// L2 translation table
typedef l2_entry page_table[0x100];

// TTBCR.N (3-bit field, range [0..7]): VA[31:32-N] == 0 → TTBR0 (user); else → TTBR1 (kernel).
// Kernel at 0xC0000000 and devices at 0xCF000000 are in TTBR1 range.
#define TTBCR_N 7u
// Number of 1MB sections in the process (TTBR0) L1 table: 2^(12-N).
constexpr u32 PROC_VA_MB = (1u << (12u - TTBCR_N));
// Required alignment for TTBR0 base with TTBCR_N (bytes): 2^(14-N).
constexpr u32 PROC_TABLE_ALIGN = (1u << (14u - TTBCR_N));
// Byte size of the process L1 table.
constexpr u32 PROC_TABLE_SIZE = (PROC_VA_MB * sizeof(l1_entry));

// Table located at TTBR1 (kernel virtual address space).
extern translation_table kernel_translation_table;

// Configures the MMU and L1 translation table, ID-mapping flash, RAM, and UART address spaces.
void init_mmu(void *dtb);

// Map a 1MB physical section as identity (VA == PA). Flushes TLB.
void mmu_map_identity(u32 phys_mb, bool device);

// Allocate a process L1 table (PROC_VA_MB entries, zero-initialized).
l1_entry *mmu_alloc_proc_table(void);

void mmu_free_proc_table(l1_entry *tt);

// Map a 1MB section at va_mb into an L1 table.
void mmu_map_section(l1_entry *tt, u32 va_mb, u32 pa_mb, bool device);

// Allocate a zeroed L2 page table (256 × 4KB small-page entries, 1KB-aligned).
l2_entry *mmu_alloc_l2_table(void);
void      mmu_free_l2_table(l2_entry *pt);

// Install an L2 table into the L1 entry for va_mb.
void mmu_attach_l2(l1_entry *tt, u32 va_mb, l2_entry *pt);

// Map one 4KB page: va and pa must be PAGE_SIZE-aligned.
void mmu_map_page(l2_entry *pt, u32 va, u32 pa);

// Switch TTBR0 to a process table (virtual address, converted to physical). Flushes TLB.
void mmu_set_proc_table(l1_entry *tt);

#endif // MEMORY_H
