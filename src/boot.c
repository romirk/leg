#include "boot.h"

#include "linker.h"
#include "types.h"

// [[clang::no_builtin]]
[[gnu::section(".startup.c")]]
void kboot() {
    // copy binary to RAM
    auto len = kernel_main_end - kernel_main_beg;
    u64 *dp64 = (void *) kernel_main_beg;
    const u64 *sp64 = (void *) kernel_load_beg;

    // copies an entire instruction at a time
    while (len >= 8) {
        *dp64++ = *sp64++;
        len -= 8;
    }

    u8 *dp8 = (void *) dp64;
    const u8 *sp8 = (void *) sp64;
    while (len--)
        *dp8++ = *sp8++;


    // set vtable address
    extern unsigned char vtable[];
    asm("mcr p15, 0, %0, c12, c0, 0" :: "r"(vtable));

    // jump
    asm("ldr pc, =kmain");
}
