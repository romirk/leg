//
// Created by Romir Kulshrestha on 01/06/2025.
//

#include "uart.h"
#include "types.h"

static void wait_tx_complete() {
    while (*UARTFR & FR_BUSY);
}

static void calculate_divisors(uint32_t *integer, uint32_t *fractional, uint32_t base_clock, uint32_t baudrate) {
    // 64 * F_UARTCLK / (16 * B) = 4 * F_UARTCLK / B
    const uint32_t div = 4 * base_clock / baudrate;

    *fractional = div & 0x3f;
    *integer = (div >> 6) & 0xffff;
}

void pl011_setup(struct pl011 *dev, uint64_t base_clock) {
    dev->base_address = UART0_BASE;
    dev->base_clock = base_clock;

    dev->baudrate = 115200;
    dev->data_bits = 8;
    dev->stop_bits = 1;

    wait_tx_complete();
    pl011_reset(dev);
}

void pl011_reset(struct pl011 *dev) {
    auto cr = *UARTCR;
    auto lcr = *UARTLCR_H;
    u32 ibrd, fbrd;

    // disable UART
    *UARTCR = cr & CR_UARTEN;

    // wait for any ongoing transmissions to complete
    wait_tx_complete();

    // Flush FIFOs
    *UARTLCR_H = lcr & ~LCR_FEN;

    // set frequency divisors to control speed
    calculate_divisors(&ibrd, &fbrd, dev->base_clock, dev->baudrate);
    *UARTIBRD = ibrd;
    *UARTFBRD = fbrd;

    // data frame format
    lcr = 0x0;
    // get WLEN
    lcr |= ((dev->data_bits - 1) & 0x3) << 5;
    // Configure the number of stop bits
    if (dev->stop_bits == 2)
        lcr |= LCR_STP2;

    // Mask all interrupts by setting corresponding bits to 1
    *UARTIMSC = 0x7ff;

    // Disable DMA by setting all bits to 0
    *UARTDMACR = 0x0;

    // I only need transmission, so that's the only thing I enabled.
    *UARTCR = CR_TXEN;

    // Finally enable UART
    *UARTCR = CR_TXEN | CR_UARTEN;
}

void putchar(char c) { *UARTDR = c; }
