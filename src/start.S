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
    .syntax unified

    @ Rockchip's U-Boot is built as Thumb code and its "go" always enters us in Thumb
    @ state (it jumps to addr|1). The rest of the kernel is ARM code, so the first two
    @ instructions are Thumb: "bx pc" reads pc as _start+4 with bit 0 clear, which
    @ switches the CPU to ARM state at _start+4. _start is marked as a Thumb function so
    @ QEMU (which starts at the ELF entry address) enters it the same way.
    .thumb
    .global _start
    .thumb_func
_start:
    bx      pc
    nop

    .arm
arm_entry:
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
    @ U-Boot runs with caches on. Throw away our instructions from the I-cache so the next
    @ build loaded at this address is fetched fresh from memory, not from stale cache lines.
    mov     r0, #0
    mcr     p15, 0, r0, c7, c5, 0   @ ICIALLU: invalidate entire instruction cache
    mcr     p15, 0, r0, c7, c5, 6   @ BPIALL: invalidate branch predictor
    dsb
    isb

    mov     r0, #0              @ return code 0, shown by U-Boot as "rc = 0x0"
    mov     sp, r4              @ restore U-Boot's stack and return to its prompt
    bx      r5
#endif
