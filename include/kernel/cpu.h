#pragma once

#ifdef __aarch64__
#include "kernel/arch/aarch64/cpu.h"
#else
#include "kernel/arch/arm32/cpu.h"
#endif
