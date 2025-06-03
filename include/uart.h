//
// Created by Romir Kulshrestha on 01/06/2025.
//

#ifndef UART_H
#define UART_H

#define UART0_BASE (volatile u32 *) 0x09000000

void putchar(char c);

#endif // UART_H
