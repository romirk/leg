//
// Created by Romir Kulshrestha on 01/06/2025.
// Reference: https://krinkinmu.github.io/2020/11/29/PL011.html

#ifndef UART_H
#define UART_H

/// UART base address
#define UART0_BASE 0x09000000
#define UART_REG(off) (volatile uint32_t *)(UART0_BASE + (off))

// UART registers

/// data register
#define UARTDR      UART_REG(0x000)
/// flag register
#define UARTFR      UART_REG(0x018)
/// Integer Baud Rate Register
#define UARTIBRD    UART_REG(0x024)
/// Fractional Baud Rate Register
#define UARTFBRD    UART_REG(0x028)
/// Line Control Register
#define UARTLCR_H   UART_REG(0x02C)
/// Control Register
#define UARTCR      UART_REG(0x030)
// Interrupt Mask Set/Clear Register
#define UARTIMSC    UART_REG(0x038)
/// DMA Control Register
#define UARTDMACR   UART_REG(0x048)

// status signals
#define FR_BUSY     (u32)(1 << 3)

#define CR_TXEN     (u32)(1 << 8)
#define CR_UARTEN   (u32)(1 << 0)

#define LCR_FEN     (u32)(1 << 4)
#define LCR_STP2    (u32)(1 << 3)

#include "types.h"

struct pl011 {
    u64 base_address;
    u64 base_clock;
    u32 baudrate;
    u32 data_bits;
    u32 stop_bits;
};

void pl011_setup(struct pl011 *dev, uint64_t base_clock);

void pl011_reset(const struct pl011 *dev);

void putchar(char c);

#endif // UART_H
