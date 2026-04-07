//
// Created by Romir Kulshrestha on 03/06/2025.
//

#ifndef STDIO_H
#define STDIO_H

#include "types.h"

// Provided by the kernel (e.g. UART driver)
void putchar(char c);

char getchar(void);

int print(const char *);

int puts(const char *);

void printf(const char *, ...);

void pprintf(const char *, ...);

void hexdump(const void *, u32);

#endif //STDIO_H
