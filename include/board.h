/* Per-board hardware addresses. Pick a board with -DBOARD_LUCKFOX or -DBOARD_QEMU. */
#ifndef BOARD_H
#define BOARD_H

#if defined(BOARD_LUCKFOX)
/* Luckfox Pico (RV1103). Addresses from the board's device tree, see spec/board.txt */
#define BOARD_NAME   "luckfox-pico (rv1103)"
#define UART_BASE    0xFF4C0000u   /* UART2, Synopsys DW 16550, regs 4 bytes apart */
#define GIC_BASE     0xFF1F0000u   /* GIC-400, used from stage 3 */

#elif defined(BOARD_QEMU)
/* qemu-system-arm -M virt -cpu cortex-a7 */
#define BOARD_NAME   "qemu virt (cortex-a7)"
#define UART_BASE    0x09000000u   /* ARM PL011 */
#define GIC_BASE     0x08000000u

#else
#error "define BOARD_LUCKFOX or BOARD_QEMU"
#endif

#endif
