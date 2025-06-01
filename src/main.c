//
// Created by Romir Kulshrestha on 01/06/2025.
//

#include "main.h"

#include "builtins.h"
#include "uart.h"

[[noreturn]]
void kmain() {
    println("Hello World!");

    for (;;) {
        asm("wfi");
    }
}
