//
// Created by Romir Kulshrestha on 01/06/2025.
//

#include "uart.h"
#include <stdint.h>

[[gnu::used, clang::no_builtin]]
void putchar(char c) {
    *(volatile uint32_t *) (UART0_BASE) = c;
}

void print(char *s) {
    while (*s) {
        putchar(*s++);
    }
}

void println(char *s) {
    print(s);
    putchar('\n');
}
