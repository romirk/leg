//
// Created by Romir Kulshrestha on 08/05/2026.
//

#include "types.h"

#define UARTDR (volatile u8 *) 0x09000000

[[gnu::section(".boot.rodata")]]
static const char msg[] = "hello world!\n";

[[gnu::section(".boot")]]
void kboot(uptr) {
    for (const char *c = msg; *c; c++) {
        *UARTDR = *c;
    }
}
