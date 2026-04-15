#include "utils.h"

#include "kernel/linker.h"
#include "kernel/logs.h"
#include "libc/stdio.h"

#include <stdarg.h>

[[noreturn]]
void panic(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    kprint("\033[97;101m PANIC \033[0m ");
    vkprintf(fmt, ap);
    va_end(ap);
    kprint("\n");

    void      *p            = nullptr;
    void      *sp           = &p;
    const auto stack_height = (void *) STACK_BOTTOM - sp;
    kprintf("sp=%p\n", sp);
    hexdump(sp, stack_height > 64 ? 64 : stack_height);

    poweroff();
}