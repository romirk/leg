#pragma once

#ifdef __aarch64__
#include "kernel/arch/aarch64/memory.h"
#else
#include "kernel/arch/arm32/memory.h"
#endif
