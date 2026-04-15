#include "libc/cstring.h"
#include "libc/stdio.h"
#include "syscall.h"
#include "usr/mandelbrot.h"

int main(void) {
    auto pid = sys_fork();
    if (pid == 0) {
        loop {
            // ReSharper disable once CppDFAEndlessLoop
            sys_uart_write("ping\n", 5);
            sys_sleep(1000000);
        }
    }
    for (int i = 0; i < 5; i++) {
        sys_uart_write("pong\n", 5);
        sys_sleep(1500000);
    }
    return 0;
}