//
// Created by Romir Kulshrestha on 01/06/2025.
//

#ifndef LINKER_H
#define LINKER_H

#define STACK_BOTTOM (void *) 0x50000000

extern unsigned char kernel_main_off[];
extern unsigned char kernel_main_end[];
extern unsigned char kernel_load_off[];

#endif // LINKER_H
