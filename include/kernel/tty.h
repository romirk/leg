#ifndef TTY_H
#define TTY_H

#include "types.h"

// Called from IRQ context (kbd, uart) with a raw character.
// Handles echo, backspace, and line buffering.
void tty_input(char c);

// Output a character to framebuffer (kernel-internal; user programs use putchar via sys_write).
void tty_putchar(char c);

// Block until a complete line is available (caller must have IRQs enabled).
u32  tty_readline(char *buf, u32 max);
char tty_getchar(void);
char tty_getchar_nb(void);

#endif // TTY_H