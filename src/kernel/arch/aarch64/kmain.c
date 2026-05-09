//
// Created by Romir Kulshrestha on 09/05/2026.
//

#include "kernel/arch/aarch64/tt.h"
#include "kernel/exceptions.h"
#include "types.h"
#include "utils.h"

#define UARTDR (volatile u8 *) 0x09000000

[[noreturn]]
void kmain() {
    install_vtable();

    constexpr char msg[] = "kmain reached\n";
    for (const char *c = msg; *c; c++) {
        *UARTDR = *c;
    }

    unmap_identity();

    limbo;
}
