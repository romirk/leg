//
// Created by Romir Kulshrestha on 10/04/2026.
//

#include "types.h"
#include "kernel/mem/alloc.h"

typedef struct {
    u32  capacity;
    u32  size;
    u32 *ptr;
} Vec;

Vec vec_create(const u32 capacity) {
    const auto ptr = kmalloc(capacity * sizeof(u32));
    return (Vec) {.capacity = ptr ? capacity : 0, .size = 0, .ptr = ptr};
}

u32 vec_push(Vec *vec, const u32 val) {
    if (vec->size >= vec->capacity) {
        vec->capacity *= 2;
        vec->ptr = krealloc(vec->ptr, vec->capacity * sizeof(u32));
        if (!vec->ptr) {
            vec->capacity = vec->size = 0;
            return 0; // OOM
        }
    }
    vec->ptr[vec->size++] = val;
    return vec->size;
}

u32 vec_pop(Vec *vec) {
    if (vec->size > 0) {
        auto val = vec->ptr[--vec->size];
        return val;
    }
    return 0;
}

u32 vec_iter(const Vec *vec, u32 (*fn)(u32)) {
    for (u32 i = 0; i < vec->size; i++) {
        if (!fn(vec->ptr[i])) return 0;
    }
    return 1;
}

void vec_free(Vec *vec) {
    kfree(vec->ptr);
    vec->capacity = 0;
    vec->size = 0;
}