//
// Created by Romir Kulshrestha on 20/08/2025.
//

#include "kernel/main.h"

#include "libc/stdio.h"

// cat: read lines and echo them back
void cat(void) {
    char buf[256];
    u32 pos = 0;

    printf("cat> ");
    for (;;) {
        char c = getchar();
        if (c == '\r' || c == '\n') {
            putchar('\r');
            putchar('\n');
            buf[pos] = '\0';
            printf("%s\n", buf);
            pos = 0;
            printf("cat> ");
        } else if (c == 127 || c == '\b') {
            if (pos > 0) {
                --pos;
                print("\b \b");
            }
        } else if (pos < sizeof(buf) - 1) {
            buf[pos++] = c;
            putchar(c);
        }
    }
}

[[gnu::noinline]]
int main() {
    return 0;
}
