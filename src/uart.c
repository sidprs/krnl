/*
 * Polling UART output. No interrupts, no init: on the Luckfox, U-Boot already set the
 * baud rate, clocks and pins, and QEMU's PL011 works out of the box. We just wait until
 * the transmitter has room, then write one byte.
 */
#include "board.h"
#include "kernel.h"

#if defined(BOARD_LUCKFOX)
/* 16550-style UART, registers are 4 bytes apart (device tree: reg-shift = 2) */
#define UART_THR   (UART_BASE + 0x00)   /* Transmit Holding Register */
#define UART_LSR   (UART_BASE + 0x14)   /* Line Status Register (reg 5 * 4) */
#define LSR_THRE   (1u << 5)            /* THR empty: ok to write */

static int tx_ready(void) { return mmio_read(UART_LSR) & LSR_THRE; }
static void tx_byte(char c) { mmio_write(UART_THR, (uint8_t)c); }

#elif defined(BOARD_QEMU)
/* ARM PL011 */
#define UART_DR    (UART_BASE + 0x00)   /* Data Register */
#define UART_FR    (UART_BASE + 0x18)   /* Flag Register */
#define FR_TXFF    (1u << 5)            /* TX FIFO full */

static int tx_ready(void) { return !(mmio_read(UART_FR) & FR_TXFF); }
static void tx_byte(char c) { mmio_write(UART_DR, (uint8_t)c); }
#endif

void uart_putc(char c)
{
    if (c == '\n')
        uart_putc('\r');          /* terminals want CR+LF */
    while (!tx_ready())
        ;
    tx_byte(c);
}

void uart_puts(const char *s)
{
    while (*s)
        uart_putc(*s++);
}
