#pragma once
// mm.h — physical page allocator + kernel heap

#include <stdint.h>
#include <stddef.h>

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
    uint32_t  total_pages;
    uint32_t  free_pages;
    uint32_t  reserved_pages; // kernel image + page tables + bitmap
    uintptr_t heap_base;
    uintptr_t heap_end;
    uintptr_t heap_used;
} mm_stats_t;

// Initialise from DTB /memory node. reserved_end is the first usable byte after
// all early-boot reservations (DTB blob, bump heap, etc.).
void mm_init(uintptr_t dtb_mem_base, uint64_t dtb_mem_size, uintptr_t reserved_end);

// Allocate one 4KB physical page. Returns PA or 0 on OOM.
uintptr_t mm_page_alloc(void);

// Allocate n contiguous physical pages. Returns base PA or 0 on OOM.
uintptr_t mm_page_alloc_n(uint32_t n);

mm_err_t mm_page_free(uintptr_t pa);
mm_err_t mm_page_free_n(uintptr_t pa, uint32_t n);

// Reserve [pa, pa+size) so it is never returned by the allocator (MMIO, ROM, etc.).
mm_err_t mm_reserve(uintptr_t pa, size_t size);

bool mm_page_is_allocated(uintptr_t pa);
void mm_stats(mm_stats_t *out);

// General-purpose kernel heap. Always 8-byte aligned; returns NULL on failure.
void *kmalloc(size_t size);
void *kzalloc(size_t size); // zeroed
void  kfree(void *ptr);
void *krealloc(void *ptr, size_t size);
