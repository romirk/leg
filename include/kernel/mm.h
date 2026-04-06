#pragma once

// =============================================================================
// mm.h — Physical Page Allocator + Kernel Heap
// Target: ARMv7a bare-metal kernel, C23, clang/GCC extensions
//
// Memory layout assumed (low → high):
//
//   [RAM_BASE]
//      ... (may be 0x00000000 or 0x80000000 etc — board-specific)
//      [_start]          kernel .text / .rodata / .data / .bss
//      [.tt section]     translation_table + page_tables  (placed by linker)
//      [_end]            ← allocator starts here
//      [heap]            bump region for early/kernel-heap allocs
//      [page bitmap]     1 bit per 4 KB page covering all RAM
//      [free pages]      everything above, managed by the page allocator
//   [RAM_END]
//
// Two allocators are provided:
//
//   1. Physical page allocator (mm_page_*)
//      Manages 4 KB physical pages using a flat bitmap.
//      Used to back new mappings, kernel stacks, etc.
//
//   2. Kernel heap (kmalloc / kfree)
//      A first-fit free-list allocator over a fixed early-boot region.
//      Used for small kernel objects (DTB tree, etc.)
//      Does NOT call into the page allocator — it operates on a pre-carved
//      contiguous region so it's safe before the page bitmap is set up.
//
// =============================================================================

#include <stdint.h>
#include <stddef.h>

// ---------------------------------------------------------------------------
// Board / build-time knobs
// ---------------------------------------------------------------------------

// RAM base and size are discovered at runtime from DTB.
// mm_init() must be called with the values from the DTB /memory node.

// Page size (ARMv7 small page)
#define PAGE_SIZE  4096u
#define PAGE_SHIFT   12u
#define PAGE_MASK  (~(PAGE_SIZE - 1u))

// Heap carved out of RAM right after _end for kmalloc
#ifndef KHEAP_SIZE
#  define KHEAP_SIZE (2u * 1024u * 1024u)   // 2 MiB default
#endif

// ---------------------------------------------------------------------------
// Error codes
// ---------------------------------------------------------------------------
typedef enum : int {
    MM_OK          =  0,
    MM_ERR_OOM     = -1,   // out of physical pages
    MM_ERR_ALIGN   = -2,   // address not page-aligned
    MM_ERR_RANGE   = -3,   // address outside managed RAM
    MM_ERR_INUSE   = -4,   // page already allocated
    MM_ERR_FREE    = -5,   // page already free (double-free)
} mm_err_t;

// ---------------------------------------------------------------------------
// Physical memory statistics
// ---------------------------------------------------------------------------
typedef struct {
    uint32_t total_pages;
    uint32_t free_pages;
    uint32_t reserved_pages;   // kernel image + page tables + bitmap
    uintptr_t heap_base;
    uintptr_t heap_end;
    uintptr_t heap_used;
} mm_stats_t;

// ---------------------------------------------------------------------------
// Initialise the memory manager.
//
// Call once, early in boot, after:
//   • BSS is zeroed
//   • MMU is enabled with the initial page tables
//   • Serial/debug output is available (optional but recommended)
//
// `dtb_mem_base` / `dtb_mem_size` are the values read from the DTB /memory
// node.  If you haven't parsed the DTB yet, pass 0 for both and the
// allocator will use RAM_BASE / RAM_SIZE from the build-time knobs.
//
// `reserved_end` is the first usable byte in RAM after all early-boot
// reservations (DTB blob, bump allocator, etc.).  The bitmap and kernel
// heap are placed starting here.
// ---------------------------------------------------------------------------
void mm_init(uintptr_t dtb_mem_base, uint64_t dtb_mem_size,
             uintptr_t reserved_end);

// ---------------------------------------------------------------------------
// Physical page allocator
// ---------------------------------------------------------------------------

// Allocate one 4 KB physical page.  Returns physical address or 0 on OOM.
uintptr_t mm_page_alloc(void);

// Allocate `n` *contiguous* physical pages.  Returns base PA or 0 on OOM.
uintptr_t mm_page_alloc_n(uint32_t n);

// Free a page previously returned by mm_page_alloc[_n].
mm_err_t mm_page_free(uintptr_t pa);

// Free `n` contiguous pages.
mm_err_t mm_page_free_n(uintptr_t pa, uint32_t n);

// Reserve a physical range [pa, pa+size) so it will never be returned by
// the allocator (use for MMIO, ROM, etc.).
mm_err_t mm_reserve(uintptr_t pa, size_t size);

// Query whether a physical address is currently allocated.
bool mm_page_is_allocated(uintptr_t pa);

// Fill `out` with current stats.
void mm_stats(mm_stats_t *out);

// ---------------------------------------------------------------------------
// Kernel heap  (kmalloc / kfree)
// ---------------------------------------------------------------------------

// General-purpose allocator. Returns NULL on failure. Always 8-byte aligned.
void *kmalloc(size_t size);

// Same but zeroes the allocation.
void *kzalloc(size_t size);

// Free a pointer returned by kmalloc / kzalloc.
void  kfree(void *ptr);

// Reallocate.  If ptr is NULL, equivalent to kmalloc(size).
void *krealloc(void *ptr, size_t size);
