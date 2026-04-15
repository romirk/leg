#include "libc/stdio.h"
#include "syscall.h"
#include "usr/hash.h"
#include "usr/mandelbrot.h"

int main(void) {
    mandelbrot(-2.0, -1.0, 1.0, 1.0);
    sys_sleep(5000000);
    // hash_run();
    return 0;
}