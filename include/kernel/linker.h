//
// Created by Romir Kulshrestha on 01/06/2025.
//

#ifndef LINKER_H
#define LINKER_H

extern unsigned char STACK_BOTTOM[];

extern unsigned char boot_vtbl_beg[];
extern unsigned char boot_vtbl_end[];

extern unsigned char kernel_main_beg[];
extern unsigned char kernel_main_end[];
extern unsigned char kernel_load_beg[];

extern unsigned char tt_l1_base[];

#endif // LINKER_H