//
// Created by Romir Kulshrestha on 01/06/2025.
//

#ifndef UART_H
#define UART_H

/// UART base address
#define UART0_BASE 0x09000000

// UART registers

/// data register
#define UARTDR      (volatile uint32_t *)(UART0_BASE + 0x000)
/// flag register
#define UARTFR      (volatile uint32_t *)(UART0_BASE + 0x018)
/// Integer Baud Rate Register
#define UARTIBRD    (volatile uint32_t *)(UART0_BASE + 0x024)
/// Fractional Baud Rate Register
#define UARTFBRD    (volatile uint32_t *)(UART0_BASE + 0x028)
/// Line Control Register
#define UARTLCR_H   (volatile uint32_t *)(UART0_BASE + 0x02C)
/// Control Register
#define UARTCR      (volatile uint32_t *)(UART0_BASE + 0x030)
// Interrupt Mask Set/Clear Register
#define UARTIMSC    (volatile uint32_t *)(UART0_BASE + 0x038)
/// DMA Control Register
#define UARTDMACR   (volatile uint32_t *)(UART0_BASE + 0x048)

// status signals
#define FR_BUSY     (u32)(1 << 3)

#define CR_TXEN     (u32)(1 << 8)
#define CR_UARTEN   (u32)(1 << 0)

#define LCR_FEN     (u32)(1 << 4)
#define LCR_STP2    (u32)(1 << 3)

int pl011_reset();


void putchar(char c);

#endif // UART_H
