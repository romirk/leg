//
// Created by Romir Kulshrestha on 03/06/2025.
//

#ifndef STDIO_H
#define STDIO_H

#include "types.h"

#ifdef NO_ANSI
#define hexdump hexdump_buffer
#else
#define hexdump hexdump_ansi
#endif

int print(const char *);

int puts(const char *);

void printf(const char *, ...);
void pprintf(const char *, ...);

void hexdump(const void *, u32);

#endif //STDIO_H
