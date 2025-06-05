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
    u8 : 4;
    bool J: 1;
    u8 ITh: 2;
    bool Q: 1;
    bool V: 1;
    bool C: 1;
    bool Z: 1;
    bool N: 1;
};

struct [[gnu::packed]] scr {
    bool NS: 1;
    bool IRQ: 1;
    bool FIQ: 1;
    bool EA: 1;
    bool FW: 1;
    bool AW: 1;
    bool nET: 1;
    bool SCD: 1;
    bool HCE: 1;
    bool SIF: 1;
    u32 : 22;
};

struct [[gnu::packed]] vbar {
    u8 : 5;
    u32 addr: 27;
};

struct [[gnu::packed]] cbar {
    u8 PERIPHBASE_39_32;
    u8 : 7;
    u32 PERIPHBASE_31_15: 17;
};

struct [[gnu::packed]] sctlr {
    bool M: 1;
    bool A: 1;
    bool C: 1;
    u8 : 2;
    bool CP15BEN: 1;
    bool : 1;
    bool B: 1;
    u8 : 2;
    bool SW: 1;
    bool Z: 1;
    bool I: 1;
    bool V: 1;
    bool RR: 1;
    bool : 1;
    bool : 1;
    bool HA: 1;
    bool : 1;
    bool WXN: 1;
    bool UWXN: 1;
    bool FI: 1;
    bool U: 1;
    bool : 1;
    bool VE: 1;
    bool EE: 1;
    bool: 1;
    bool NMFI: 1;
    bool TRE: 1;
    bool AFE: 1;
    bool TE: 1;
    bool: 1;
};

struct cpsr read_cpsr(void);

struct scr read_scr(void);

struct vbar read_vbar(void);

struct sctlr read_sctlr(void);

u32 read_periphbase_39_15(void);

void write_cpsr(struct cpsr);

void write_scr(struct scr);

void write_vbar(struct vbar);

void write_sctlr(struct sctlr);

#endif //CPU_H
