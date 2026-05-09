#pragma once

#include "types.h"
uptr svc_dispatch(uptr r0, uptr r1, uptr r2, uptr r3, uptr svc_num);

#ifdef __aarch64__
#include "kernel/arch/aarch64/syscall.h"
#else
#include "kernel/arch/arm32/syscall.h"
#endif
