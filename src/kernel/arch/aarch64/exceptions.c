//
// Created by Romir Kulshrestha on 09/05/2026.
//

#include "kernel/arch/aarch64/exceptions.h"
#include "types.h"

extern byte vectors[];

void install_vtable() {
    asm volatile("msr vbar_el1, %0" ::"r"(vectors));
}
