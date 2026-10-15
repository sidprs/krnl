/*
 * Entry point. The first instruction of the kernel image.
 *
 * On the Luckfox, U-Boot's "go <addr>" calls us like a C function: r0 = argc, r1 = argv,
 * lr = where to return in U-Boot. We save U-Boot's sp/lr, switch to our own stack,
 * zero .bss, run kmain, then return to the "=>" prompt — so you can load the next build
 * without power-cycling the board.
 *
 * On QEMU there is nothing to return to, so we halt instead.
 */
    .section .text.boot
    .global _start
_start:
    cpsid   if                  @ mask IRQ and FIQ: we have no handlers yet

    mov     r4, sp              @ r4-r7 are callee-saved, so they survive the C calls below
    mov     r5, lr
    mov     r6, r0              @ argc from U-Boot
    mov     r7, r1              @ argv from U-Boot

    ldr     sp, =__stack_top    @ our own stack (grows down), defined in link.ld

    @ C expects global variables without an initializer to start at 0
    ldr     r0, =__bss_start
    ldr     r1, =__bss_end
    mov     r2, #0
1:  cmp     r0, r1
    strlo   r2, [r0], #4
    blo     1b

    mov     r0, r6
    mov     r1, r7
    mov     r2, r4              @ pass U-Boot's sp so kmain can print it
    bl      kmain               @ kmain(argc, argv, caller_sp)

#ifdef BOARD_QEMU
2:  wfe                         @ nothing to return to: sleep forever
    b       2b
#else
    mov     sp, r4              @ restore U-Boot's stack and return to its prompt
    bx      r5
#endif
