//
// Created by Romir Kulshrestha on 12/05/2026.
//

#pragma once
#include "types.h"

void timer_set_oneshot_us(u32 usec);

void timer_advance_cval(u32 usec);

void timer_disable(void);
