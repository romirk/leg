#pragma once

#ifdef __aarch64__
#include "kernel/arch/aarch64/syscall.h"
#else
#include "kernel/arch/arm32/syscall.h"
#endif
