//
// Created by Romir Kulshrestha on 03/06/2025.
//

#ifndef STDIO_H
#define STDIO_H

#include "types.h"

int print(const char *);

int puts(const char *);

void printf(const char *, ...);

void hexdump(const void *, u32);

#endif //STDIO_H
