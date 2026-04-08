#include "kernel/boot.h"

#include "kernel/linker.h"
#include "kernel/mem/mmu.h"
#include "types.h"

[[gnu::section(".startup.boot")]] [[noreturn]]
void kboot(void *dtb) {
    // Init MMU
    init_mmu(dtb);

    // copy kernel code/data from ROM to virtual address
    auto      len = kernel_main_end - kernel_main_beg;
    u8       *dp8 = (void *) kernel_main_beg;
    const u8 *sp8 = (void *) kernel_load_beg;
    while (len--)
        *dp8++ = *sp8++;

    // zero BSS (tt section is NOLOAD and already written by map_sections)
    for (u8 *p = bss_beg; p < bss_end; p++)
        *p = 0;

    // set kernel_phys_base now that kernel is mapped
    kernel_phys_base = ((u32) dtb & ~0xFFFFF) + KPHYS_OFFSET;

    // set vtable base address
    extern unsigned char vtable[];
    asm("mcr p15, 0, %0, c12, c0, 0" ::"r"(vtable));

    // move the stack pointer
    asm volatile("ldr sp, =STACK_BOTTOM - 0x1000");

    // remove flash from translation
    kernel_translation_table[0x000].raw = 0x00000000;

    // pass DTB pointer to kmain in r0
    register void *r0 asm("r0") = dtb;
    asm volatile("ldr pc, =kmain" ::"r"(r0) : "pc");

    __builtin_unreachable();
}
