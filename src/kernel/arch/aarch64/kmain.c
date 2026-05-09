//
// Created by Romir Kulshrestha on 09/05/2026.
//

#include "types.h"
#define UARTDR (volatile u8 *) 0x09000000

[[noreturn]]
void kmain() {
    constexpr char msg[] = "kmain reached\n";
    for (const char *c = msg; *c; c++) {
        *UARTDR = *c;
    }

    // ReSharper disable once CppDFAEndlessLoop
    for (;;) {
        asm volatile("wfi");
    }
}
