//
// Created by Romir Kulshrestha on 03/06/2025.
//

#ifndef STDIO_H
#define STDIO_H

#include "types.h"

int print(const char *);

int puts(const char *s);

void printf(const char *, ...);

void hexdump(u32 *addr, u32 len);

#endif //STDIO_H
