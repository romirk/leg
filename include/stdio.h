//
// Created by Romir Kulshrestha on 03/06/2025.
//

#ifndef STDIO_H
#define STDIO_H

#include "types.h"
#include "logs.h"

void print(const char *);

void println(const char *s);

void print_u32(u32);

void print_i32(i32);

void print_hex(u32);

void print_ptr(void *ptr);

void print_bool(bool);

void printf(const char *, ...);

void hexdump(u32 *addr, u32 len);

#endif //STDIO_H
