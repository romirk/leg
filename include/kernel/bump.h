//
// Created by Romir Kulshrestha on 02/03/2026.
//

#ifndef LEG_BUMP_H
#define LEG_BUMP_H

#include "types.h"

void early_malloc_init(void *base, u32 size);

void *early_malloc(u32 size);

void early_malloc_reset(void);

#endif //LEG_BUMP_H
