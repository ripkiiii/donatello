# Makefile — build and run Donatello.
#
#   make        build the kernel image (build/donatello.bin)
#   make run    boot it in QEMU
#   make clean  remove build artifacts

CC  := i686-elf-gcc
AS  := i686-elf-as

CFLAGS  := -std=gnu99 -ffreestanding -O2 -Wall -Wextra -Iinclude
LDFLAGS := -ffreestanding -O2 -nostdlib

BUILD  := build
KERNEL := $(BUILD)/donatello.bin

# Let make find sources in their folders.
vpath %.c kernel cpu drivers lib mm
vpath %.s boot cpu

# Objects (flattened into build/). boot.o first out of habit; the linker
# script is what actually places the multiboot header at the front.
OBJS := $(BUILD)/boot.o \
        $(BUILD)/kernel.o \
        $(BUILD)/gdt.o $(BUILD)/gdt_flush.o \
        $(BUILD)/idt.o $(BUILD)/idt_load.o $(BUILD)/isr.o \
        $(BUILD)/irq.o \
        $(BUILD)/terminal.o $(BUILD)/keyboard.o \
        $(BUILD)/shell.o $(BUILD)/string.o \
        $(BUILD)/pmm.o $(BUILD)/paging.o

all: $(KERNEL)

$(BUILD):
	mkdir -p $(BUILD)

# Link everything into one kernel image using our memory map.
$(KERNEL): $(OBJS) linker.ld
	$(CC) -T linker.ld -o $@ $(OBJS) $(LDFLAGS) -lgcc

$(BUILD)/%.o: %.c | $(BUILD)
	$(CC) -c $< -o $@ $(CFLAGS)

$(BUILD)/%.o: %.s | $(BUILD)
	$(AS) $< -o $@

# Boot the kernel directly — QEMU acts as the Multiboot bootloader.
run: $(KERNEL)
	qemu-system-i386 -kernel $(KERNEL)

clean:
	rm -rf $(BUILD)

.PHONY: all run clean
