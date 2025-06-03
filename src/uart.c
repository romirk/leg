//
// Created by Romir Kulshrestha on 01/06/2025.
//

#include "uart.h"
#include "types.h"

void putchar(char c) { *UART0_BASE = c; }
