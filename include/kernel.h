#ifndef KERNEL_H
#define KERNEL_H

#include <stdint.h>

/* Memory-mapped register access. volatile = the compiler must really do every read/write. */
static inline uint32_t mmio_read(uintptr_t addr)              { return *(volatile uint32_t *)addr; }
static inline void     mmio_write(uintptr_t addr, uint32_t v) { *(volatile uint32_t *)addr = v; }

/* uart.c */
void uart_putc(char c);
void uart_puts(const char *s);

/* kprintf.c — supports %s %c %d %u %x %p %% */
void kprintf(const char *fmt, ...);

#endif
