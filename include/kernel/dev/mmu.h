#pragma once

#ifdef __aarch64__
#include "kernel/arch/aarch64/mmu.h"
#else
#include "kernel/arch/arm32/mmu.h"
#endif
