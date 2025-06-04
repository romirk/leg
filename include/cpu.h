//
// Created by Romir Kulshrestha on 04/06/2025.
//

#ifndef CPU_H
#define CPU_H
#include "types.h"

enum processor_mode {
    usr = 0b10000,
    fiq = 0b10001,
    irq = 0b10010,
    svc = 0b10011,
    mon = 0b10110,
    abt = 0b10111,
    hyp = 0b11010,
    und = 0b11011,
    sys = 0b11111,
};

struct [[gnu::packed]] cpsr {
    u8 M: 5;
    bool T: 1;
    bool F: 1;
    bool I: 1;
    bool A: 1;
    bool E: 1;
    u8 ITl: 6;
    u8 GE: 4;
    u8 _reserved: 4;
    bool J: 1;
    u8 ITh: 2;
    bool Q: 1;
    bool V: 1;
    bool C: 1;
    bool Z: 1;
    bool N: 1;
};

struct cpsr get_cpsr();

enum processor_mode get_processor_mode();

#endif //CPU_H
