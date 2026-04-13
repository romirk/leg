#include "utils.h"

#include "kernel/linker.h"
#include "kernel/logs.h"
#include "libc/stdio.h"

[[noreturn]]
void panic(char *msg) {
    err("PANIC: %s", msg);

    err("debug info:\nstack: *");
    void      *p = nullptr;
    void      *sp = &p;
    const auto stack_height = (void *) STACK_BOTTOM - sp;

    kprintf("%p\n", sp);
    hexdump(sp, stack_height > 64 ? 64 : stack_height);

    poweroff();
}
