#include "boot.h"

#include "linker.h"
#include "memory.h"
#include "types.h"

[[gnu::section(".startup.boot")]]
[[noreturn]]
void kboot() {
    // Init MMU
    init_mmu();

    // copy binary to RAM
    auto len = kernel_main_end - kernel_main_beg;
    u8 *dp8 = (void *) kernel_main_beg;
    const u8 *sp8 = (void *) kernel_load_beg;
    while (len--)
        *dp8++ = *sp8++;

    // set vtable base address
    extern unsigned char vtable[];
    asm ("mcr p15, 0, %0, c12, c0, 0" :: "r"(vtable));

    // move the stack pointer
    asm volatile ("ldr sp, =STACK_BOTTOM - 0x1000");

    // remove flash from translation
    kernel_translation_table[0x000].raw = 0x00000000;

    // jump
    asm ("ldr pc, =kmain");

    // prevent the compiler from believing we can return from here
    __builtin_unreachable();
}
