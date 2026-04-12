#include "kernel/dev/uart.h"
#include "kernel/tty.h"

static void wait_tx_complete() {
    while (*UARTFR & FR_BUSY)
        ;
}

static void calculate_divisors(u32 *integer, u32 *fractional, const u32 base_clock,
                               const u32 baudrate) {
    // 64 * F_UARTCLK / (16 * B) = 4 * F_UARTCLK / B
    const u32 div = 4 * base_clock / baudrate;

    *fractional = div & 0x3f;
    *integer = div >> 6 & 0xffff;
}

void pl011_setup(struct pl011 *dev, const u64 base_clock) {
    dev->base_address = UART0_BASE;
    dev->base_clock = base_clock;

    dev->baudrate = 115200;
    dev->data_bits = 8;
    dev->stop_bits = 1;

    wait_tx_complete();
    pl011_reset(dev);
}

void pl011_reset(const struct pl011 *dev) {
    const auto cr = *UARTCR;
    auto       lcr = *UARTLCR_H;
    u32        ibrd, fbrd;

    // disable UART
    *UARTCR = cr & CR_UARTEN;

    // wait for any ongoing transmissions to complete
    wait_tx_complete();

    // flush FIFOs
    *UARTLCR_H = lcr & ~LCR_FEN;

    // set baud rate divisors
    calculate_divisors(&ibrd, &fbrd, dev->base_clock, dev->baudrate);
    *UARTIBRD = ibrd;
    *UARTFBRD = fbrd;

    // data frame format
    lcr = 0x0;
    lcr |= (dev->data_bits - 1 & 0x3) << 5;
    if (dev->stop_bits == 2) lcr |= LCR_STP2;

    // disable DMA
    *UARTDMACR = 0x0;

    // clear all pending interrupts
    *UARTICR = 0x7FF;

    // enable RX interrupt only
    *UARTIMSC = MSC_RXIM;

    // enable FIFOs + line control
    lcr |= LCR_FEN;
    *UARTLCR_H = lcr;

    // enable UART
    *UARTCR = CR_TXEN | CR_RXEN | CR_UARTEN;
}

void uart_putchar(const char c) {
    *UARTDR = c;
}

char uart_getchar(void) {
    while (*UARTFR & (1 << 4))
        ; // RXFE: 1 = FIFO empty
    return (char) (*UARTDR & 0xFF);
}
int uart_puts(const char *s) {
    const char *p = s;
    while (*p) {
        uart_putchar(*p++);
    }
    return p - s;
}

void uart_irq_handler(void) {
    while (!(*UARTFR & (1 << 4))) {
        char c = (char) (*UARTDR & 0xFF);
        tty_input(c);
    }
    *UARTICR = MSC_RXIM;
}
