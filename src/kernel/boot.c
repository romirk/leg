#include "boot.h"

#include "linker.h"
#include "memory.h"
#include "types.h"

[[gnu::section(".startup.c")]]
[[noreturn]]
void kboot() {
    init_mmu();
    // copy binary to RAM
    auto len = kernel_main_end - kernel_main_beg;
    u8 *dp8 = 0x40000000 + (void *) kernel_main_beg;
    const u8 *sp8 = (void *) kernel_load_beg;
    while (len--)
        *dp8++ = *sp8++;

    // set vtable address
    extern unsigned char vtable[];
    asm("mcr p15, 0, %0, c12, c0, 0" :: "r"(vtable));

    // move the stack pointer
    asm volatile ("ldr sp, =0x00100000 - 0x1000");

    // remap address space 0x000 back to ram
    auto t = kernel_translation_table[0x400];
    kernel_translation_table[0x400].raw = 0;
    kernel_translation_table[0x000] = t;

    // jump
    asm ("ldr pc, =kmain");

    // prevent the compiler from believing we can return from here
    __builtin_unreachable();
}
