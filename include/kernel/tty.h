#ifndef TTY_H
#define TTY_H

#include "types.h"

// Called from IRQ context (kbd, uart) with a raw character.
// Handles echo, backspace, and line buffering.
void tty_input(char c);

// Output a character to framebuffer.
void putchar(char c);

// Block until a complete line is available (caller must have IRQs enabled).
// Copies the line into buf (including the trailing \n), null-terminates, returns byte count.
u32  readline(char *buf, u32 max);
char getchar();
char getchar_nonblocking();

#endif // TTY_H