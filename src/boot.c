#include "boot.h"

#include "linker.h"

[[clang::no_builtin]]
[[gnu::section(".startup")]]
void kboot() {
    // copy binary to RAM
    const auto len = kernel_main_end - kernel_main_off;
    const auto start = kernel_load_off;
    for (auto i = 0; i < len; i++) {
        kernel_main_off[i] = start[i];
    }

    // jump
    asm("ldr pc, =kmain");
}
