//
// Created by Romir Kulshrestha on 02/03/2026.
//

#include "kernel/bump.h"

static u8 *early_heap_base;
static u8 *early_heap_end;
static u32 early_heap_ptr = 0;

void early_malloc_init(void *base, u32 size) {
    early_heap_base = (u8 *) base;
    early_heap_end = early_heap_base + size;
    early_heap_ptr = 0;
}

void* early_malloc(u32 size) {
    u8 *current_ptr = early_heap_base + early_heap_ptr;

    if (current_ptr + size > early_heap_end) {
        return nullptr;
    }
    early_heap_ptr += size;
    return current_ptr;
}

void early_malloc_reset(void) {
    early_heap_ptr = 0;
}
