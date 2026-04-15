// start.c — user-space entry point
//
// _user_entry is placed at the very start of the user binary (section
// .text.entry), so its VA equals PROC_CODE_VA (0x00400000).  process_exec
// sets lr_usr = sys_exit_stub, so returning from main() calls sys_exit(0).

#include "syscall.h"

extern int main(void);

[[gnu::section(".text.entry"), gnu::noreturn]]
void _user_entry(void) {
    sys_exit(main());
}
