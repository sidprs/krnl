/*
 * Where each part of the kernel goes in memory.
 * LOAD_ADDR is passed in by the Makefile (-Wl,--defsym=LOAD_ADDR=...), and must match
 * the address you give U-Boot's loady/go (or QEMU's RAM).
 */
ENTRY(_start)

SECTIONS
{
    . = LOAD_ADDR;

    .text : {
        KEEP(*(.text.boot))     /* _start must be the very first byte */
        *(.text*)
    }
    .rodata : { *(.rodata*) }
    .data   : { *(.data*) }

    .bss (NOLOAD) : ALIGN(4) {
        __bss_start = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end = .;
    }

    . = ALIGN(16);
    . += 0x4000;                /* 16 KB kernel stack */
    __stack_top = .;

    /DISCARD/ : { *(.ARM.exidx*) *(.ARM.attributes) *(.comment) }
}
