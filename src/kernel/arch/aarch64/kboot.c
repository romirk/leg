//
// Created by Romir Kulshrestha on 08/05/2026.
//

#include "types.h"
#include "kernel/arch/aarch64/exceptions.h"
#include "kernel/dev/memory.h"
#include "kernel/dev/mmu.h"

[[gnu::section(".boot"), noreturn]]
void kboot(uptr) {
    init_mmu();

    install_vtable();

    extern void kmain(void);
    asm volatile("add sp, sp, %0" ::"r"(KERNEL_VA_OFFSET));
    asm volatile("br %0" ::"r"(kmain));
    __builtin_unreachable();
}
