#include "../include/uart.h"

[[gnu::naked]]
void vtable(void) {
    asm("b handle_reset");
}

void start() {
    println("Hello World!");
    // println((char*)0x80000000);
}

void handle_reset(void) {
    start();
}
