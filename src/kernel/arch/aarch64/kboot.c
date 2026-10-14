//
// Created by Romir Kulshrestha on 08/05/2026.
//

#include "types.h"
#include "kernel/kmain.h"
#include "kernel/arch/aarch64/exceptions.h"
#include "kernel/dev/memory.h"
#include "kernel/dev/mmu.h"

[[gnu::aligned(0x1000)]]
byte BOOT_STACK[BOOT_STACK_SIZE];

static volatile uptr kmain_addr = (uptr) kmain;

[[noreturn]]
void kboot(uptr) {
    init_mmu();

    install_vtable();

    asm volatile("add sp, sp, %0" ::"r"(PAGE_OFFSET));
    asm volatile("br %0" ::"r"(kmain_addr));
    __builtin_unreachable();
}
