//
// Created by Romir Kulshrestha on 01/06/2025.
//

#ifndef UART_H
#define UART_H

#define UART0_BASE 0x09000000

void putchar(char c);

void print(char *s);

void println(char *s);

#endif //UART_H
