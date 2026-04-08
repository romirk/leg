// alloc.c — physical page allocator + kernel heap

#include "kernel/mem/alloc.h"

#include "utils.h"
#include "libc/builtins.h"

// bitmap: 1 bit per page, 1 = allocated/reserved, 0 = free
typedef u32 bm_word_t;
#define BM_BITS (sizeof(bm_word_t) * 8u)

static void bm_set(bm_word_t *bm, u32 bit) {
    bm[bit / BM_BITS] |= (1u << (bit % BM_BITS));
}

static void bm_clear(bm_word_t *bm, u32 bit) {
    bm[bit / BM_BITS] &= ~(1u << (bit % BM_BITS));
}

static bool bm_test(const bm_word_t *bm, u32 bit) {
    return (bm[bit / BM_BITS] >> (bit % BM_BITS)) & 1u;
}

// returns start of first run of n free bits at or after start_bit, or U32_MAX
static u32 bm_find_free_run(const bm_word_t *bm, const u32 total_bits, const u32 start_bit,
                            const u32 n) {
    u32 run_start = start_bit;
    u32 run_len = 0;
    for (u32 i = start_bit; i < total_bits; ++i) {
        if (!bm_test(bm, i)) {
            if (run_len == 0) run_start = i;
            ++run_len;
            if (run_len == n) return run_start;
        } else {
            run_len = 0;
        }
    }
    // wrap: retry from bit 0
    if (start_bit > 0) {
        run_len = 0;
        for (u32 i = 0; i < start_bit; ++i) {
            if (!bm_test(bm, i)) {
                if (run_len == 0) run_start = i;
                ++run_len;
                if (run_len == n) return run_start;
            } else {
                run_len = 0;
            }
        }
    }
    return U32_MAX;
}

typedef struct kheap_hdr kheap_hdr_t;

// header prepended to every heap block (free or allocated)
struct [[maybe_unused]] kheap_hdr {
    u32          magic;     // KHEAP_MAGIC_FREE or KHEAP_MAGIC_USED
    u32          size;      // usable bytes following this header
    kheap_hdr_t *next_free; // next block in free list (allocated blocks: NULL)
};

// kernel heap instance
typedef struct {
    u8          *base;      // start of the heap region
    size_t       capacity;  // total bytes in the region
    size_t       used;      // bytes currently allocated (excl. headers)
    kheap_hdr_t *free_list; // head of the free-block list (address-ordered)
} kheap_t;

static kheap_t g_kheap;

static void kheap_init(uptr base, size_t size);

// physical memory manager state
typedef struct {
    uptr       ram_base; // first page-aligned byte of managed RAM
    uptr       ram_end;  // one past last managed byte
    u32        total_pages;
    bm_word_t *bitmap; // one bit per page; lives in RAM just after reserved_end
    u32        bitmap_words;
    u32        free_pages;
    u32        reserved_pages;
    u32        next_hint; // roving start index for O(1) amortised alloc
} phys_mm_t;

static phys_mm_t g_pmm;

static u32 pa_to_idx(const uptr pa) {
    return (pa - g_pmm.ram_base) >> PAGE_SHIFT;
}

static uptr idx_to_pa(const u32 idx) {
    return g_pmm.ram_base + (idx << PAGE_SHIFT);
}

static bool pa_in_range(const uptr pa) {
    return pa >= g_pmm.ram_base && pa < g_pmm.ram_end;
}

static bool pa_aligned(const uptr pa) {
    return (pa & (PAGE_SIZE - 1u)) == 0;
}

// called only during init; no double-reserve check
static void reserve_range(uptr base, uptr end) {
    base = align_down(base, PAGE_SIZE);
    end = align_up(end, PAGE_SIZE);
    if (base < g_pmm.ram_base) base = g_pmm.ram_base;
    if (end > g_pmm.ram_end) end = g_pmm.ram_end;
    for (uptr pa = base; pa < end; pa += PAGE_SIZE) {
        const u32 idx = pa_to_idx(pa);
        if (!bm_test(g_pmm.bitmap, idx)) {
            bm_set(g_pmm.bitmap, idx);
            ++g_pmm.reserved_pages;
            if (g_pmm.free_pages > 0) --g_pmm.free_pages;
        }
    }
}

void mm_init(const uptr dtb_mem_base, const u64 dtb_mem_size, const uptr reserved_end) {
    ASSERT(dtb_mem_size > 0, "mm_init: no RAM info from DTB");

    // clamp to 32-bit PA space (ARMv7a)
    const u64 end64 = (u64) dtb_mem_base + dtb_mem_size;
    uptr      ram_base = dtb_mem_base;
    uptr      ram_end = (end64 > U32_MAX) ? U32_MAX & PAGE_MASK : (uptr) end64;
    ram_base = align_up(ram_base, PAGE_SIZE);
    ram_end = align_down(ram_end, PAGE_SIZE);

    ASSERT(ram_end > ram_base, "mm_init: degenerate RAM range");

    g_pmm.ram_base = ram_base;
    g_pmm.ram_end = ram_end;
    g_pmm.total_pages = (ram_end - ram_base) >> PAGE_SHIFT;
    g_pmm.next_hint = 0;
    g_pmm.free_pages = g_pmm.total_pages;
    g_pmm.reserved_pages = 0;

    const uptr bitmap_pa = align_up(reserved_end, PAGE_SIZE);
    g_pmm.bitmap_words = div_round_up(g_pmm.total_pages, BM_BITS);
    const size_t bitmap_bytes = g_pmm.bitmap_words * sizeof(bm_word_t);

    ASSERT(bitmap_pa + bitmap_bytes <= ram_end, "mm_init: bitmap overflows RAM");

    g_pmm.bitmap = (bm_word_t *) bitmap_pa;
    memset(g_pmm.bitmap, 0, bitmap_bytes);

    reserve_range(ram_base, bitmap_pa); // DTB, bump heap, etc.
    reserve_range(bitmap_pa, bitmap_pa + bitmap_bytes);
    const uptr heap_pa = align_up(bitmap_pa + bitmap_bytes, PAGE_SIZE);
    reserve_range(heap_pa, heap_pa + KHEAP_SIZE);

    kheap_init(heap_pa, KHEAP_SIZE);
}

uptr mm_page_alloc(void) {
    if (g_pmm.free_pages == 0) return 0;
    const u32 idx = bm_find_free_run(g_pmm.bitmap, g_pmm.total_pages, g_pmm.next_hint, 1);
    if (idx == U32_MAX) return 0;
    bm_set(g_pmm.bitmap, idx);
    --g_pmm.free_pages;
    g_pmm.next_hint = (idx + 1) % g_pmm.total_pages;
    return idx_to_pa(idx);
}

uptr mm_page_alloc_n(const u32 n) {
    if (n == 0) return 0;
    if (g_pmm.free_pages < n) return 0;
    const u32 idx = bm_find_free_run(g_pmm.bitmap, g_pmm.total_pages, g_pmm.next_hint, n);
    if (idx == U32_MAX) return 0;
    for (u32 i = 0; i < n; ++i)
        bm_set(g_pmm.bitmap, idx + i);
    g_pmm.free_pages -= n;
    g_pmm.next_hint = (idx + n) % g_pmm.total_pages;
    return idx_to_pa(idx);
}

mm_err_t mm_page_free(const uptr pa) {
    if (!pa_aligned(pa)) return MM_ERR_ALIGN;
    if (!pa_in_range(pa)) return MM_ERR_RANGE;
    const u32 idx = pa_to_idx(pa);
    if (!bm_test(g_pmm.bitmap, idx)) return MM_ERR_FREE;
    bm_clear(g_pmm.bitmap, idx);
    ++g_pmm.free_pages;
    if (idx < g_pmm.next_hint) g_pmm.next_hint = idx;
    return MM_OK;
}

mm_err_t mm_page_free_n(const uptr pa, const u32 n) {
    if (!pa_aligned(pa)) return MM_ERR_ALIGN;
    if (!pa_in_range(pa)) return MM_ERR_RANGE;
    for (u32 i = 0; i < n; ++i) {
        const u32 idx = pa_to_idx(pa + i * PAGE_SIZE);
        if (!bm_test(g_pmm.bitmap, idx)) return MM_ERR_FREE;
        bm_clear(g_pmm.bitmap, idx);
        ++g_pmm.free_pages;
    }
    if (pa_to_idx(pa) < g_pmm.next_hint) g_pmm.next_hint = pa_to_idx(pa);
    return MM_OK;
}

bool mm_page_is_allocated(const uptr pa) {
    if (!pa_aligned(pa) || !pa_in_range(pa)) return false;
    return bm_test(g_pmm.bitmap, pa_to_idx(pa));
}

void mm_stats(mm_stats_t *out) {
    if (!out) return;
    const uptr heap_base = (uptr) g_kheap.base;
    out->total_pages = g_pmm.total_pages;
    out->free_pages = g_pmm.free_pages;
    out->reserved_pages = g_pmm.reserved_pages;
    out->heap_base = heap_base;
    out->heap_end = heap_base + g_kheap.capacity;
    out->heap_used = g_kheap.used;
}

// kernel heap — first-fit free-list
// each block: [kheap_hdr_t | user data], size = usable bytes (excl. header)

#define KHEAP_MIN_SPLIT  32u
#define KHEAP_ALIGN      8u
#define KHEAP_MAGIC_FREE 0xCAFEu
#define KHEAP_MAGIC_USED 0xBEEFu
#define HDR_SIZE         (sizeof(kheap_hdr_t))

static void kheap_init(uptr base, size_t size) {
    ASSERT(size > HDR_SIZE + KHEAP_MIN_SPLIT, "kheap: region too small");

    g_kheap.base = (u8 *) base;
    g_kheap.capacity = size;
    g_kheap.used = 0;

    const auto h = (kheap_hdr_t *) base;
    h->magic = KHEAP_MAGIC_FREE;
    h->size = size - HDR_SIZE;
    h->next_free = nullptr;
    g_kheap.free_list = h;
}

static kheap_hdr_t *hdr_of(void *ptr) {
    return (kheap_hdr_t *) ((u8 *) ptr - HDR_SIZE);
}

static bool hdr_valid(const kheap_hdr_t *h, u32 expected_magic) {
    const uptr haddr = (uptr) h;
    const uptr base = (uptr) g_kheap.base;
    if (haddr < base || haddr + HDR_SIZE > base + g_kheap.capacity) return false;
    return h->magic == expected_magic;
}

void *kmalloc(size_t size) {
    if (size == 0) return nullptr;
    size = (size + KHEAP_ALIGN - 1u) & ~(KHEAP_ALIGN - 1u);

    kheap_hdr_t *prev = nullptr;
    kheap_hdr_t *cur = g_kheap.free_list;

    while (cur) {
        ASSERT(cur->magic == KHEAP_MAGIC_FREE, "kmalloc: heap corruption (bad free-list magic)");

        if (cur->size >= size) {
            if (cur->size >= size + HDR_SIZE + KHEAP_MIN_SPLIT) {
                // split: carve a free block from the tail
                const auto split = (kheap_hdr_t *) ((u8 *) cur + HDR_SIZE + size);
                split->magic = KHEAP_MAGIC_FREE;
                split->size = cur->size - size - HDR_SIZE;
                split->next_free = cur->next_free;
                cur->size = size;
                cur->next_free = split;
                if (prev)
                    prev->next_free = split;
                else
                    g_kheap.free_list = split;
            } else {
                if (prev)
                    prev->next_free = cur->next_free;
                else
                    g_kheap.free_list = cur->next_free;
            }
            cur->magic = KHEAP_MAGIC_USED;
            cur->next_free = nullptr;
            g_kheap.used += cur->size;
            return (u8 *) cur + HDR_SIZE;
        }
        prev = cur;
        cur = cur->next_free;
    }
    return nullptr;
}

void *kzalloc(const size_t size) {
    void *p = kmalloc(size);
    if (p) memset(p, 0, size);
    return p;
}

void kfree(void *ptr) {
    if (!ptr) return;

    kheap_hdr_t *h = hdr_of(ptr);
    ASSERT(hdr_valid(h, KHEAP_MAGIC_USED), "kfree: invalid or double-free");

    h->magic = KHEAP_MAGIC_FREE;
    g_kheap.used -= h->size;

    // insert in address order (enables coalescing)
    kheap_hdr_t *prev = nullptr;
    kheap_hdr_t *cur = g_kheap.free_list;
    while (cur && cur < h) {
        prev = cur;
        cur = cur->next_free;
    }

    h->next_free = cur;
    if (prev)
        prev->next_free = h;
    else
        g_kheap.free_list = h;

    // coalesce with next
    if (h->next_free) {
        kheap_hdr_t *next = h->next_free;
        const u8    *expected_next = (u8 *) h + HDR_SIZE + h->size;
        if ((u8 *) next == expected_next && next->magic == KHEAP_MAGIC_FREE) {
            h->size += HDR_SIZE + next->size;
            h->next_free = next->next_free;
            next->magic = 0xDEADu; // poison absorbed header
        }
    }

    // coalesce with previous
    if (prev) {
        const u8 *expected_h = (u8 *) prev + HDR_SIZE + prev->size;
        if ((u8 *) h == expected_h && prev->magic == KHEAP_MAGIC_FREE) {
            prev->size += HDR_SIZE + h->size;
            prev->next_free = h->next_free;
            h->magic = 0xDEADu;
        }
    }
}

void *krealloc(void *ptr, const size_t size) {
    if (!ptr) return kmalloc(size);
    if (!size) {
        kfree(ptr);
        return nullptr;
    }

    const kheap_hdr_t *h = hdr_of(ptr);
    ASSERT(hdr_valid(h, KHEAP_MAGIC_USED), "krealloc: invalid pointer");

    const size_t rounded = (size + KHEAP_ALIGN - 1u) & ~(KHEAP_ALIGN - 1u);
    if (h->size >= rounded) return ptr;

    void *new_ptr = kmalloc(size);
    if (!new_ptr) return nullptr;
    memcpy(new_ptr, ptr, h->size < size ? h->size : size);
    kfree(ptr);
    return new_ptr;
}
