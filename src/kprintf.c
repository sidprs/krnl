/* A tiny printf. There is no C library in a kernel, so we write our own. */
#include <stdarg.h>
#include "kernel.h"

static void print_uint(uint32_t v, unsigned base, int width)
{
    char buf[11];
    int i = 0;

    do {
        buf[i++] = "0123456789abcdef"[v % base];
        v /= base;
    } while (v);
    while (i < width)
        buf[i++] = '0';
    while (i)
        uart_putc(buf[--i]);
}

void kprintf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    for (; *fmt; fmt++) {
        if (*fmt != '%') {
            uart_putc(*fmt);
            continue;
        }
        switch (*++fmt) {
        case 's': { const char *s = va_arg(ap, const char *); uart_puts(s ? s : "(null)"); break; }
        case 'c': uart_putc((char)va_arg(ap, int)); break;
        case 'u': print_uint(va_arg(ap, uint32_t), 10, 0); break;
        case 'd': {
            int32_t v = va_arg(ap, int32_t);
            if (v < 0) { uart_putc('-'); v = -v; }
            print_uint((uint32_t)v, 10, 0);
            break;
        }
        case 'x': print_uint(va_arg(ap, uint32_t), 16, 0); break;
        case 'p': uart_puts("0x"); print_uint(va_arg(ap, uint32_t), 16, 8); break;
        case '%': uart_putc('%'); break;
        case '\0': fmt--; break;   /* lone % at end of string */
        default: uart_putc('%'); uart_putc(*fmt); break;
        }
    }
    va_end(ap);
}
