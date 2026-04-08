#pragma once
// alloc.h — physical page allocator + kernel heap

#include "types.h"

// page size for ARMv7 small pages
#define PAGE_SIZE  4096u
#define PAGE_SHIFT 12u
#define PAGE_MASK  (~(PAGE_SIZE - 1u))

// heap region carved from RAM right after early-boot reservations
#ifndef KHEAP_SIZE
#define KHEAP_SIZE (2u * 1024u * 1024u)
#endif

typedef enum : int {
    MM_OK = 0,
    MM_ERR_OOM = -1,   // out of physical pages
    MM_ERR_ALIGN = -2, // address not page-aligned
    MM_ERR_RANGE = -3, // address outside managed RAM
    MM_ERR_INUSE = -4, // page already allocated
    MM_ERR_FREE = -5,  // double-free
} mm_err_t;

typedef struct {
    u32  total_pages;
    u32  free_pages;
    u32  reserved_pages; // kernel image + page tables + bitmap
    uptr heap_base;
    uptr heap_end;
    uptr heap_used;
} mm_stats_t;

// Initialize from DTB /memory node. reserved_end is the first usable byte after
// all early-boot reservations (DTB blob, bump heap, etc.).
void mm_init(uptr dtb_mem_base, u64 dtb_mem_size, uptr reserved_end);

// Allocate one 4KB physical page. Returns PA or 0 on OOM.
uptr mm_page_alloc(void);

// Allocate n contiguous physical pages. Returns base PA or 0 on OOM.
uptr mm_page_alloc_n(u32 n);

// Free a single physical page. Returns MM_ERR_ALIGN/RANGE if invalid.
mm_err_t mm_page_free(uptr pa);
// Free n contiguous pages starting at pa.
mm_err_t mm_page_free_n(uptr pa, u32 n);

// Returns true if the page at pa is currently allocated.
bool mm_page_is_allocated(uptr pa);
// Fill out with current allocator and heap statistics.
void mm_stats(mm_stats_t *out);

// General-purpose kernel heap. Always 8-byte aligned; returns NULL on failure.
void *kmalloc(size_t size);
// Like kmalloc but zero-initializes the returned block.
void *kzalloc(size_t size);
// Free a block previously returned by kmalloc/kzalloc/krealloc.
void kfree(void *ptr);
// Resize an allocation; semantics match standard realloc.
void *krealloc(void *ptr, size_t size);