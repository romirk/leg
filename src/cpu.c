//
// Created by Romir Kulshrestha on 04/06/2025.
//

#include "cpu.h"

enum processor_mode get_processor_mode() {
    auto cpsr = get_cpsr();
    return cpsr.M;
}

struct cpsr get_cpsr() {
    struct cpsr cpsr;
    asm ("mrs %0, cpsr" : "=r"(cpsr));
    return cpsr;
}
