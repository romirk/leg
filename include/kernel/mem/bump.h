#pragma once

#include "types.h"

void early_malloc_init(void *base, u32 size);

void *early_malloc(u32 size);

void early_malloc_reset(void);
