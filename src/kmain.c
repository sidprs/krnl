/* Stage 1: prove we are running, and look at the state the CPU was handed to us in. */
#include "board.h"
#include "kernel.h"

extern char _start[], __bss_start[], __bss_end[], __stack_top[];

/* CPSR: current program status register. Low 5 bits = CPU mode. */
static uint32_t read_cpsr(void)
{
    uint32_t v;
    __asm__ volatile("mrs %0, cpsr" : "=r"(v));
    return v;
}

/* CP15 registers are read with mrc: coprocessor 15 holds the system control registers. */
static uint32_t read_midr(void)   /* Main ID Register: which CPU is this? */
{
    uint32_t v;
    __asm__ volatile("mrc p15, 0, %0, c0, c0, 0" : "=r"(v));
    return v;
}

static uint32_t read_sctlr(void)  /* System Control Register: MMU and cache switches */
{
    uint32_t v;
    __asm__ volatile("mrc p15, 0, %0, c1, c0, 0" : "=r"(v));
    return v;
}

static const char *mode_name(uint32_t mode)
{
    switch (mode) {
    case 0x10: return "USR";
    case 0x11: return "FIQ";
    case 0x12: return "IRQ";
    case 0x13: return "SVC";
    case 0x16: return "MON";
    case 0x17: return "ABT";
    case 0x1a: return "HYP";
    case 0x1b: return "UND";
    case 0x1f: return "SYS";
    default:   return "???";
    }
}

void kmain(int argc, char **argv, uint32_t caller_sp)
{
    (void)argv;
    uint32_t cpsr = read_cpsr();
    uint32_t midr = read_midr();
    uint32_t sctlr = read_sctlr();

    kprintf("\n\nHello from krnl!\n");
    kprintf("board     : %s\n", BOARD_NAME);
    kprintf("loaded at : %p  (bss %p-%p, stack top %p)\n",
            (uint32_t)_start, (uint32_t)__bss_start, (uint32_t)__bss_end, (uint32_t)__stack_top);
    kprintf("caller sp : %p  argc=%d\n", caller_sp, argc);
    kprintf("cpu mode  : %s (cpsr=%p)\n", mode_name(cpsr & 0x1f), cpsr);
    kprintf("cpu id    : %p  (%s)\n", midr,
            ((midr >> 4) & 0xfff) == 0xc07 ? "Cortex-A7" : "unknown");
    kprintf("sctlr     : %p  MMU %s, D-cache %s, I-cache %s\n", sctlr,
            (sctlr & (1u << 0))  ? "on" : "off",
            (sctlr & (1u << 2))  ? "on" : "off",
            (sctlr & (1u << 12)) ? "on" : "off");
    kprintf("bye.\n");
}
