// =============================================================================
// mm.c — Physical Page Allocator + Kernel Heap
// ARMv7a bare-metal, C23, no libc
// =============================================================================

#include "kernel/mm.h"

#include "utils.h"
#include "libc/stdlib.h"

// ---------------------------------------------------------------------------
// Minimal intrinsics
// ---------------------------------------------------------------------------
static inline void *k_memset(void *dst, int c, size_t n) {
    uint8_t *p = (uint8_t *)dst;
    while (n--) *p++ = (uint8_t)c;
    return dst;
}
static inline void *k_memcpy(void *dst, const void *src, size_t n) {
    uint8_t *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;
    while (n--) *d++ = *s++;
    return dst;
}

#define MM_ASSERT(cond, msg)  do { if (!(cond)) panic(msg); } while(0)

// ---------------------------------------------------------------------------
// Utility: align up/down
// ---------------------------------------------------------------------------
static inline uintptr_t align_up(uintptr_t x, uintptr_t align) {
    return (x + align - 1u) & ~(align - 1u);
}
static inline uintptr_t align_down(uintptr_t x, uintptr_t align) {
    return x & ~(align - 1u);
}
static inline uint32_t div_round_up(uint32_t a, uint32_t b) {
    return (a + b - 1u) / b;
}

// ---------------------------------------------------------------------------
// Bitmap helpers  (1 bit per page; 1 = allocated/reserved, 0 = free)
// ---------------------------------------------------------------------------
typedef uint32_t bm_word_t;
#define BM_BITS  (sizeof(bm_word_t) * 8u)

static inline void bm_set(bm_word_t *bm, uint32_t bit) {
    bm[bit / BM_BITS] |= (1u << (bit % BM_BITS));
}
static inline void bm_clear(bm_word_t *bm, uint32_t bit) {
    bm[bit / BM_BITS] &= ~(1u << (bit % BM_BITS));
}
static inline bool bm_test(const bm_word_t *bm, uint32_t bit) {
    return (bm[bit / BM_BITS] >> (bit % BM_BITS)) & 1u;
}

// Find the first run of `n` consecutive zero bits starting at or after
// `start_bit`.  Returns UINT32_MAX if not found.
static uint32_t bm_find_free_run(const bm_word_t *bm, uint32_t total_bits,
                                 uint32_t start_bit, uint32_t n) {
    uint32_t run_start = start_bit;
    uint32_t run_len   = 0;
    for (uint32_t i = start_bit; i < total_bits; ++i) {
        if (!bm_test(bm, i)) {
            if (run_len == 0) run_start = i;
            ++run_len;
            if (run_len == n) return run_start;
        } else {
            run_len = 0;
        }
    }
    // Wrap: try from bit 0 if we started mid-bitmap
    if (start_bit > 0) {
        run_len = 0;
        for (uint32_t i = 0; i < start_bit; ++i) {
            if (!bm_test(bm, i)) {
                if (run_len == 0) run_start = i;
                ++run_len;
                if (run_len == n) return run_start;
            } else {
                run_len = 0;
            }
        }
    }
    return UINT32_MAX;
}

// ---------------------------------------------------------------------------
// Forward declarations (heap state defined after page allocator)
// ---------------------------------------------------------------------------
typedef struct kheap_hdr kheap_hdr_t;
struct kheap_hdr {
    uint32_t     magic;
    uint32_t     size;
    kheap_hdr_t *next_free;
};
typedef struct {
    uint8_t     *base;
    size_t       capacity;
    size_t       used;
    kheap_hdr_t *free_list;
} kheap_t;
static kheap_t g_kheap;
static void kheap_init(uintptr_t base, size_t size);

// ---------------------------------------------------------------------------
// Physical memory state
// ---------------------------------------------------------------------------
typedef struct {
    uintptr_t   ram_base;       // first byte of managed RAM (page-aligned)
    uintptr_t   ram_end;        // one past last managed byte
    uint32_t    total_pages;    // (ram_end - ram_base) / PAGE_SIZE
    bm_word_t  *bitmap;         // one bit per page, lives in RAM after _end
    uint32_t    bitmap_words;   // number of bm_word_t entries
    uint32_t    free_pages;
    uint32_t    reserved_pages;
    uint32_t    next_hint;      // roving pointer for O(1) amortised alloc
} phys_mm_t;

static phys_mm_t g_pmm;

// ---------------------------------------------------------------------------
// Page address ↔ page index
// ---------------------------------------------------------------------------
static inline uint32_t pa_to_idx(uintptr_t pa) {
    return (uint32_t)((pa - g_pmm.ram_base) >> PAGE_SHIFT);
}
static inline uintptr_t idx_to_pa(uint32_t idx) {
    return g_pmm.ram_base + ((uintptr_t)idx << PAGE_SHIFT);
}
static inline bool pa_in_range(uintptr_t pa) {
    return pa >= g_pmm.ram_base && pa < g_pmm.ram_end;
}
static inline bool pa_aligned(uintptr_t pa) {
    return (pa & (PAGE_SIZE - 1u)) == 0;
}

// ---------------------------------------------------------------------------
// Reserve a physical range in the bitmap (internal, no bounds check on
// double-reserve — we call this only during init).
// ---------------------------------------------------------------------------
static void _reserve_range(uintptr_t base, uintptr_t end) {
    base = align_down(base, PAGE_SIZE);
    end  = align_up(end,   PAGE_SIZE);
    if (base < g_pmm.ram_base) base = g_pmm.ram_base;
    if (end  > g_pmm.ram_end)  end  = g_pmm.ram_end;
    for (uintptr_t pa = base; pa < end; pa += PAGE_SIZE) {
        uint32_t idx = pa_to_idx(pa);
        if (!bm_test(g_pmm.bitmap, idx)) {
            bm_set(g_pmm.bitmap, idx);
            ++g_pmm.reserved_pages;
            if (g_pmm.free_pages > 0) --g_pmm.free_pages;
        }
    }
}

// ---------------------------------------------------------------------------
// mm_init
// ---------------------------------------------------------------------------
void mm_init(uintptr_t dtb_mem_base, uint64_t dtb_mem_size,
             uintptr_t reserved_end) {
    // --- Determine managed RAM window ---
    uintptr_t ram_base, ram_end;
    if (dtb_mem_size > 0) {
        ram_base = (uintptr_t)dtb_mem_base;
        // Clamp to 32-bit PA space on ARMv7a
        uint64_t end64 = (uint64_t)dtb_mem_base + dtb_mem_size;
        ram_end = (end64 > UINT32_MAX) ? UINT32_MAX & PAGE_MASK
                                       : (uintptr_t)end64;
    } else {
        panic("mm_init: no RAM info from DTB");
    }
    ram_base = align_up(ram_base,   PAGE_SIZE);
    ram_end  = align_down(ram_end,  PAGE_SIZE);

    MM_ASSERT(ram_end > ram_base, "mm_init: degenerate RAM range");

    g_pmm.ram_base    = ram_base;
    g_pmm.ram_end     = ram_end;
    g_pmm.total_pages = (uint32_t)((ram_end - ram_base) >> PAGE_SHIFT);
    g_pmm.next_hint   = 0;
    g_pmm.free_pages  = g_pmm.total_pages;
    g_pmm.reserved_pages = 0;

    // --- Place bitmap after all early-boot reservations ---
    uintptr_t bitmap_pa = align_up(reserved_end, PAGE_SIZE);

    // Bitmap size: 1 bit per page, rounded up to word boundary
    g_pmm.bitmap_words = div_round_up(g_pmm.total_pages, (uint32_t)BM_BITS);
    size_t bitmap_bytes = (size_t)g_pmm.bitmap_words * sizeof(bm_word_t);

    MM_ASSERT(bitmap_pa + bitmap_bytes <= ram_end, "mm_init: bitmap overflows RAM");

    g_pmm.bitmap = (bm_word_t *)bitmap_pa;

    // Zero the bitmap (all pages free initially)
    k_memset(g_pmm.bitmap, 0, bitmap_bytes);

    // --- Reserve: everything before the bitmap (DTB, bump heap, etc.) ---
    _reserve_range(ram_base, bitmap_pa);

    // --- Reserve: the bitmap itself ---
    _reserve_range(bitmap_pa, bitmap_pa + bitmap_bytes);

    // --- Reserve: heap region (right after bitmap) ---
    uintptr_t heap_pa = align_up(bitmap_pa + bitmap_bytes, PAGE_SIZE);
    _reserve_range(heap_pa, heap_pa + KHEAP_SIZE);

    // --- Initialise kernel heap ---
    kheap_init(heap_pa, KHEAP_SIZE);
}

// ---------------------------------------------------------------------------
// Public API: mm_reserve
// ---------------------------------------------------------------------------
mm_err_t mm_reserve(uintptr_t pa, size_t size) {
    if (!pa_aligned(pa))        return MM_ERR_ALIGN;
    if (!pa_in_range(pa))       return MM_ERR_RANGE;
    _reserve_range(pa, pa + size);
    return MM_OK;
}

// ---------------------------------------------------------------------------
// mm_page_alloc
// ---------------------------------------------------------------------------
uintptr_t mm_page_alloc(void) {
    if (g_pmm.free_pages == 0) return 0;
    uint32_t idx = bm_find_free_run(g_pmm.bitmap, g_pmm.total_pages,
                                    g_pmm.next_hint, 1);
    if (idx == UINT32_MAX) return 0;
    bm_set(g_pmm.bitmap, idx);
    --g_pmm.free_pages;
    g_pmm.next_hint = (idx + 1) % g_pmm.total_pages;
    return idx_to_pa(idx);
}

uintptr_t mm_page_alloc_n(uint32_t n) {
    if (n == 0) return 0;
    if (g_pmm.free_pages < n) return 0;
    uint32_t idx = bm_find_free_run(g_pmm.bitmap, g_pmm.total_pages,
                                    g_pmm.next_hint, n);
    if (idx == UINT32_MAX) return 0;
    for (uint32_t i = 0; i < n; ++i) bm_set(g_pmm.bitmap, idx + i);
    g_pmm.free_pages -= n;
    g_pmm.next_hint = (idx + n) % g_pmm.total_pages;
    return idx_to_pa(idx);
}

// ---------------------------------------------------------------------------
// mm_page_free
// ---------------------------------------------------------------------------
mm_err_t mm_page_free(uintptr_t pa) {
    if (!pa_aligned(pa))  return MM_ERR_ALIGN;
    if (!pa_in_range(pa)) return MM_ERR_RANGE;
    uint32_t idx = pa_to_idx(pa);
    if (!bm_test(g_pmm.bitmap, idx)) return MM_ERR_FREE;
    bm_clear(g_pmm.bitmap, idx);
    ++g_pmm.free_pages;
    if (idx < g_pmm.next_hint) g_pmm.next_hint = idx; // hint toward low memory
    return MM_OK;
}

mm_err_t mm_page_free_n(uintptr_t pa, uint32_t n) {
    if (!pa_aligned(pa))  return MM_ERR_ALIGN;
    if (!pa_in_range(pa)) return MM_ERR_RANGE;
    for (uint32_t i = 0; i < n; ++i) {
        uint32_t idx = pa_to_idx(pa + (uintptr_t)i * PAGE_SIZE);
        if (!bm_test(g_pmm.bitmap, idx)) return MM_ERR_FREE;
        bm_clear(g_pmm.bitmap, idx);
        ++g_pmm.free_pages;
    }
    if (pa_to_idx(pa) < g_pmm.next_hint) g_pmm.next_hint = pa_to_idx(pa);
    return MM_OK;
}

bool mm_page_is_allocated(uintptr_t pa) {
    if (!pa_aligned(pa) || !pa_in_range(pa)) return false;
    return bm_test(g_pmm.bitmap, pa_to_idx(pa));
}

void mm_stats(mm_stats_t *out) {
    if (!out) return;
    uintptr_t heap_base = (uintptr_t)g_kheap.base;
    out->total_pages    = g_pmm.total_pages;
    out->free_pages     = g_pmm.free_pages;
    out->reserved_pages = g_pmm.reserved_pages;
    out->heap_base      = heap_base;
    out->heap_end       = heap_base + g_kheap.capacity;
    out->heap_used      = g_kheap.used;
}

// =============================================================================
// Kernel Heap  —  first-fit free-list
//
// Layout of each block in memory:
//
//   [kheap_hdr_t | ... user data ... | (padding to 8-byte boundary)]
//
// The header is always present.  `size` is the usable data size (not
// including the header itself).  Free blocks form a singly-linked list
// via `next_free`; allocated blocks are not linked (we find them by
// scanning from `base` when freeing).
// =============================================================================

#define KHEAP_MIN_SPLIT  32u
#define KHEAP_ALIGN       8u
#define KHEAP_MAGIC_FREE  0xCAFEu
#define KHEAP_MAGIC_USED  0xBEEFu
#define HDR_SIZE  (sizeof(kheap_hdr_t))

static void kheap_init(uintptr_t base, size_t size) {
    MM_ASSERT(size > HDR_SIZE + KHEAP_MIN_SPLIT, "kheap: region too small");

    g_kheap.base     = (uint8_t *)base;
    g_kheap.capacity = size;
    g_kheap.used     = 0;

    // One giant free block covering the whole region.
    kheap_hdr_t *h  = (kheap_hdr_t *)base;
    h->magic        = KHEAP_MAGIC_FREE;
    h->size         = (uint32_t)(size - HDR_SIZE);
    h->next_free    = nullptr;
    g_kheap.free_list = h;
}

// Return pointer to header for a user pointer.
static inline kheap_hdr_t *hdr_of(void *ptr) {
    return (kheap_hdr_t *)((uint8_t *)ptr - HDR_SIZE);
}

// Verify a header is within the heap region and has a valid magic.
static inline bool hdr_valid(const kheap_hdr_t *h, uint32_t expected_magic) {
    uintptr_t haddr = (uintptr_t)h;
    uintptr_t base  = (uintptr_t)g_kheap.base;
    if (haddr < base || haddr + HDR_SIZE > base + g_kheap.capacity) return false;
    return h->magic == expected_magic;
}

void *kmalloc(size_t size) {
    if (size == 0) return nullptr;
    // Round up to KHEAP_ALIGN
    size = (size + KHEAP_ALIGN - 1u) & ~(KHEAP_ALIGN - 1u);

    kheap_hdr_t *prev = nullptr;
    kheap_hdr_t *cur  = g_kheap.free_list;

    while (cur) {
        MM_ASSERT(cur->magic == KHEAP_MAGIC_FREE, "kmalloc: heap corruption (bad free-list magic)");

        if (cur->size >= size) {
            // Can we split?
            if (cur->size >= size + HDR_SIZE + KHEAP_MIN_SPLIT) {
                // Carve a new free block from the tail.
                kheap_hdr_t *split = (kheap_hdr_t *)((uint8_t *)cur + HDR_SIZE + size);
                split->magic     = KHEAP_MAGIC_FREE;
                split->size      = cur->size - (uint32_t)size - (uint32_t)HDR_SIZE;
                split->next_free = cur->next_free;
                cur->size        = (uint32_t)size;
                cur->next_free   = split; // temporarily; unlinked below
                // Relink: prev → split → ...
                if (prev) prev->next_free = split;
                else      g_kheap.free_list = split;
            } else {
                // Take the whole block.
                if (prev) prev->next_free = cur->next_free;
                else      g_kheap.free_list = cur->next_free;
            }
            cur->magic     = KHEAP_MAGIC_USED;
            cur->next_free = nullptr;
            g_kheap.used  += cur->size;
            return (uint8_t *)cur + HDR_SIZE;
        }
        prev = cur;
        cur  = cur->next_free;
    }
    return nullptr; // OOM
}

void *kzalloc(size_t size) {
    void *p = kmalloc(size);
    if (p) k_memset(p, 0, size);
    return p;
}

void kfree(void *ptr) {
    if (!ptr) return;

    kheap_hdr_t *h = hdr_of(ptr);
    MM_ASSERT(hdr_valid(h, KHEAP_MAGIC_USED), "kfree: invalid or double-free");

    h->magic = KHEAP_MAGIC_FREE;
    g_kheap.used -= h->size;

    // Insert into free list in address order (enables coalescing).
    kheap_hdr_t *prev = nullptr;
    kheap_hdr_t *cur  = g_kheap.free_list;
    while (cur && cur < h) { prev = cur; cur = cur->next_free; }

    h->next_free = cur;
    if (prev) prev->next_free = h;
    else      g_kheap.free_list = h;

    // Coalesce with next block if physically adjacent.
    if (h->next_free) {
        kheap_hdr_t *next = h->next_free;
        uint8_t *expected_next = (uint8_t *)h + HDR_SIZE + h->size;
        if ((uint8_t *)next == expected_next && next->magic == KHEAP_MAGIC_FREE) {
            h->size     += (uint32_t)HDR_SIZE + next->size;
            h->next_free = next->next_free;
            // Poison the absorbed header so stale pointers to it are caught.
            next->magic = 0xDEADu;
        }
    }

    // Coalesce with previous block if physically adjacent.
    if (prev) {
        uint8_t *expected_h = (uint8_t *)prev + HDR_SIZE + prev->size;
        if ((uint8_t *)h == expected_h && prev->magic == KHEAP_MAGIC_FREE) {
            prev->size     += (uint32_t)HDR_SIZE + h->size;
            prev->next_free = h->next_free;
            h->magic        = 0xDEADu;
        }
    }
}

void *krealloc(void *ptr, size_t size) {
    if (!ptr)   return kmalloc(size);
    if (!size)  { kfree(ptr); return nullptr; }

    kheap_hdr_t *h = hdr_of(ptr);
    MM_ASSERT(hdr_valid(h, KHEAP_MAGIC_USED), "krealloc: invalid pointer");

    size_t rounded = (size + KHEAP_ALIGN - 1u) & ~(KHEAP_ALIGN - 1u);
    if (h->size >= rounded) return ptr;  // already fits

    void *new_ptr = kmalloc(size);
    if (!new_ptr) return nullptr;
    k_memcpy(new_ptr, ptr, h->size < size ? h->size : size);
    kfree(ptr);
    return new_ptr;
}
