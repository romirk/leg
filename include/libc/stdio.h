#ifndef STDIO_H
#define STDIO_H

#include "types.h"

void putchar(char c);

char getchar(void);

char getchar_nb(void);

// Read one line from the TTY into buf (up to max bytes). Blocks until \n. Returns byte count.
u32 readline(char *buf, u32 max);

int print(const char *);

int puts(const char *);

void printf(const char *, ...);

void pprintf(const char *, ...);

void hexdump(const void *, u32);

#endif // STDIO_H
