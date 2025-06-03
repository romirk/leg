//
// Created by Romir Kulshrestha on 01/06/2025.
//

#include "uart.h"

#include "stdlib.h"
#include "types.h"

void putchar(char c) { *UART0_BASE = c; }

int print(char *s) {
    int count = 0;
    while (*s) {
        putchar(*s++);
        count++;
    }
    return count;
}

int println(char *s) {
    auto count = print(s);
    putchar('\n');
    return count;
}

void hexdump(u32 *addr, u32 len) {
    for (u32 i = 1; i <= len; i++) {
        char res[9];
        hex32le(*addr++, res);
        print(res);
        putchar(i % 4 ? ' ' : '\n');
    }
    putchar('\n');
}

void print_ptr(void *ptr) {
    char res[9];
    hex32be((u32) ptr, res);
    print(res);
}

void print_num(i32 num) {
    char res[12];
    itoa(num, res);
    print(res);
}

