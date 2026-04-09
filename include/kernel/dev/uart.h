// uart.h — PL011 UART driver

#ifndef UART_H
#define UART_H

#define UART0_PHYSICAL 0x09000000
#define UART0_BASE     UART0_PHYSICAL
#define UART_REG(off)  (volatile u32 *) (UART0_BASE + (off))

// GIC interrupt ID: SPI 1 → GIC ID 33
#define UART_IRQ 33

// registers
#define UARTDR    UART_REG(0x000) // data
#define UARTFR    UART_REG(0x018) // flags
#define UARTIBRD  UART_REG(0x024) // integer baud rate divisor
#define UARTFBRD  UART_REG(0x028) // fractional baud rate divisor
#define UARTLCR_H UART_REG(0x02C) // line control
#define UARTCR    UART_REG(0x030) // control
#define UARTIMSC  UART_REG(0x038) // interrupt mask set/clear
#define UARTICR   UART_REG(0x044) // interrupt clear
#define UARTDMACR UART_REG(0x048) // DMA control

// flag/control bits
#define FR_BUSY (u32)(1 << 3)

#define CR_TXEN   (u32)(1 << 8)
#define CR_RXEN   (u32)(1 << 9)
#define CR_UARTEN (u32)(1 << 0)

#define LCR_FEN  (u32)(1 << 4)
#define LCR_STP2 (u32)(1 << 3)

#define MSC_RXIM (u32)(1 << 4)

#include "types.h"

// PL011 UART device state
struct pl011 {
    u64 base_address; // MMIO base (virtual)
    u64 base_clock;   // input clock frequency in Hz
    u32 baudrate;
    u32 data_bits; // 5–8
    u32 stop_bits; // 1 or 2
};

void pl011_setup(struct pl011 *dev, u64 base_clock);

void pl011_reset(const struct pl011 *dev);

void uart_irq_handler(void);

void uart_putchar(char c);
char uart_getchar(void);

#endif // UART_H
