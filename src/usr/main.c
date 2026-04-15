// user entry point

#include "usr/main.h"

#include "usr/hash.h"
#include "usr/mandelbrot.h"
#include "usr/matrix.h"

int main() {
    // hash_run();
    mandelbrot(-2, -1.5, 1, 1);
    return 0;
}
