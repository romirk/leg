//
// Created by Romir Kulshrestha on 01/06/2025.
//

#include "uart.h"
#include "types.h"
#include "logs.h"

static void wait_tx_complete() {
    while (*UARTFR * FR_BUSY != 0);
}

void calculate_divisors(uint32_t *integer, uint32_t *fractional, uint32_t base_clock, uint32_t baudrate) {
    // 64 * F_UARTCLK / (16 * B) = 4 * F_UARTCLK / B
    const uint32_t div = 4 * base_clock / baudrate;

    *fractional = div & 0x3f;
    *integer = (div >> 6) & 0xffff;
}

int pl011_reset() {
    auto cr = *UARTCR;
    auto lcr = *UARTLCR_H;
    // u32 ibrd, fbrd;

    // disable UART
    *UARTCR = cr & CR_UARTEN;

    // wait for any ongoing transmissions to complete
    wait_tx_complete();

    // Flush FIFOs
    *UARTLCR_H = lcr & ~LCR_FEN;
    return 0;
}

void putchar(char c) { *UARTDR = c; }
