# Makefile — build and run Donatello.
#
#   make        build the kernel image (build/donatello.bin)
#   make run    boot it in QEMU (with the M9 disk image attached)
#   make clean  remove build artifacts

CC      := i686-elf-gcc
AS      := i686-elf-as
LD      := i686-elf-ld

CFLAGS  := -std=gnu99 -ffreestanding -O2 -Wall -Wextra -Iinclude
LDFLAGS := -ffreestanding -O2 -nostdlib

BUILD  := build
KERNEL := $(BUILD)/donatello.bin

# Let make find sources in their folders.
vpath %.c kernel cpu drivers lib mm fs
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
        $(BUILD)/pmm.o $(BUILD)/paging.o $(BUILD)/heap.o \
        $(BUILD)/elf.o \
        $(BUILD)/usermode.o $(BUILD)/syscall.o \
        $(BUILD)/ata.o $(BUILD)/fs.o

all: $(KERNEL)

$(BUILD):
	mkdir -p $(BUILD)

# --- M9: programs now live on disk, not embedded in the kernel image -----
# hello.c (M7) and usertest.c (M8) are still separate, freestanding ELF
# binaries — that part hasn't changed. What changed is where their bytes
# END UP: instead of `ld -r -b binary` baking them into donatello.bin,
# tools/mkdisk.py packs them into disk.img, and fs_load() (fs.c) reads
# them off disk at runtime by name. This is the exact swap M7's own
# comments predicted back when the embedding trick was introduced.
$(BUILD)/hello.o: userprogs/hello.c | $(BUILD)
	$(CC) -c $< -o $@ -std=gnu99 -ffreestanding -O2 -Wall -Wextra

$(BUILD)/hello.elf: $(BUILD)/hello.o userprogs/hello.ld
	$(CC) -T userprogs/hello.ld -o $@ $< -ffreestanding -O2 -nostdlib

$(BUILD)/usertest.o: userprogs/usertest.c | $(BUILD)
	$(CC) -c $< -o $@ -std=gnu99 -ffreestanding -O2 -Wall -Wextra

$(BUILD)/usertest.elf: $(BUILD)/usertest.o userprogs/usertest.ld
	$(CC) -T userprogs/usertest.ld -o $@ $< -ffreestanding -O2 -nostdlib

# The disk image: sector 0 = file table, then each program's bytes,
# built entirely by the host-side Python tool (see tools/mkdisk.py for the
# on-disk format — it must exactly match what fs.c expects to read back).
$(BUILD)/disk.img: $(BUILD)/hello.elf $(BUILD)/usertest.elf tools/mkdisk.py
	python3 tools/mkdisk.py $@ hello=$(BUILD)/hello.elf usertest=$(BUILD)/usertest.elf

# Link everything into one kernel image using our memory map.
$(KERNEL): $(OBJS) linker.ld
	$(CC) -T linker.ld -o $@ $(OBJS) $(LDFLAGS) -lgcc

$(BUILD)/%.o: %.c | $(BUILD)
	$(CC) -c $< -o $@ $(CFLAGS)

$(BUILD)/%.o: %.s | $(BUILD)
	$(AS) $< -o $@

# Boot the kernel directly (QEMU acts as the Multiboot bootloader) with the
# M9 disk attached on the primary ATA bus — the same legacy controller
# QEMU's default PC machine always provides, independent of boot method.
run: $(KERNEL) $(BUILD)/disk.img
	qemu-system-i386 -kernel $(KERNEL) -drive file=$(BUILD)/disk.img,format=raw,if=ide

clean:
	rm -rf $(BUILD)

.PHONY: all run clean
