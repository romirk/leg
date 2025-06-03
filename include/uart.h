//
// Created by Romir Kulshrestha on 01/06/2025.
//

#ifndef UART_H
#define UART_H

#define UART0_BASE (volatile u32 *) 0x09000000
#include "types.h"

void putchar(char c);

int print(char *s);

int println(char *s);

void hexdump(u32 *addr, u32 len);

void print_ptr(void *ptr);
void print_num(i32 num);

#endif // UART_H
