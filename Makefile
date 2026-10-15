# krnl — build for the Luckfox Pico (default) or QEMU.
#
#   make            build build/luckfox/kernel.bin for the board
#   make qemu       build for QEMU virt and run it (quit: Ctrl-A then X)
#   make qemu-gdb   same, but wait for gdb on :1234
#   make dis        disassemble the board build
#   make run        load the board build over serial through U-Boot's "=> " prompt and run it
#                   (board at "=> ", picocom closed)
#   make clean

CROSS   ?= arm-none-eabi-
CC      := $(CROSS)gcc
OBJCOPY := $(CROSS)objcopy
OBJDUMP := $(CROSS)objdump

BOARD   ?= luckfox

# Load address: on the Luckfox, check it is free RAM with U-Boot's bdinfo/printenv.
# QEMU virt puts RAM at 0x40000000.
ifeq ($(BOARD),luckfox)
LOAD_ADDR ?= 0x00200000
DEFS      := -DBOARD_LUCKFOX
else ifeq ($(BOARD),qemu)
LOAD_ADDR ?= 0x40010000
DEFS      := -DBOARD_QEMU
else
$(error BOARD must be luckfox or qemu)
endif

BUILD   := build/$(BOARD)
SRCS    := $(wildcard src/*.c src/*.S)
OBJS    := $(patsubst src/%,$(BUILD)/%.o,$(SRCS))

CFLAGS  := -mcpu=cortex-a7 -marm -ffreestanding -fno-builtin -nostdlib \
           -O2 -g -Wall -Wextra -Iinclude $(DEFS)
LDFLAGS := -T link.ld -Wl,--defsym=LOAD_ADDR=$(LOAD_ADDR) -nostdlib -Wl,--build-id=none -Wl,--no-warn-rwx-segments

.PHONY: all qemu qemu-gdb dis run clean

PORT ?= /dev/cu.usbserial-210

all: $(BUILD)/kernel.bin
	@echo "built $< (load at $(LOAD_ADDR), $$(wc -c < $<) bytes)"

$(BUILD)/kernel.elf: $(OBJS) link.ld
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $@ -lgcc

$(BUILD)/kernel.bin: $(BUILD)/kernel.elf
	$(OBJCOPY) -O binary $< $@

$(BUILD)/%.o: src/% include/*.h Makefile
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

QEMU := qemu-system-arm -M virt -cpu cortex-a7 -m 64M -nographic

qemu:
	$(MAKE) BOARD=qemu all
	$(QEMU) -kernel build/qemu/kernel.elf

qemu-gdb:
	$(MAKE) BOARD=qemu all
	@echo "in another terminal: arm-none-eabi-gdb build/qemu/kernel.elf -ex 'target remote :1234'"
	$(QEMU) -kernel build/qemu/kernel.elf -s -S

run: $(BUILD)/kernel.bin
	@[ "$(BOARD)" = luckfox ] || { echo "run is for BOARD=luckfox (use make qemu)"; exit 1; }
	python3 tools/uboot-load.py $< --addr $(LOAD_ADDR) --port $(PORT) --go

dis: $(BUILD)/kernel.elf
	$(OBJDUMP) -d $< | less

clean:
	rm -rf build
