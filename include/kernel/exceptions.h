//
// Created by Romir Kulshrestha on 09/05/2026.
//

#pragma once

#ifdef __aarch64__
#include "kernel/arch/aarch64/exceptions.h"
#else
#include "kernel/arch/arm32/exceptions.h"
#endif
