# Makefile — build and run Donatello.
#
#   make        build the kernel image (build/donatello.bin)
#   make run    boot it in QEMU
#   make clean  remove build artifacts

CC      := i686-elf-gcc
AS      := i686-elf-as
LD      := i686-elf-ld

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
        $(BUILD)/pmm.o $(BUILD)/paging.o $(BUILD)/heap.o \
        $(BUILD)/elf.o $(BUILD)/hello_blob.o \
        $(BUILD)/usermode.o $(BUILD)/syscall.o $(BUILD)/usertest_blob.o

all: $(KERNEL)

$(BUILD):
	mkdir -p $(BUILD)

# --- M7: the embedded test program --------------------------------------
# A separate, freestanding ELF binary, linked to run at 2 MiB (userprogs/
# hello.ld) — nothing to do with the kernel's own toolchain flags above.
# `ld -r -b binary` treats hello.elf as an opaque blob and wraps it in an
# object file with linker-generated symbols (_binary_hello_elf_start/_end)
# marking where its bytes begin and end — that's what elf_load() reads.
$(BUILD)/hello.o: userprogs/hello.c | $(BUILD)
	$(CC) -c $< -o $@ -std=gnu99 -ffreestanding -O2 -Wall -Wextra

$(BUILD)/hello.elf: $(BUILD)/hello.o userprogs/hello.ld
	$(CC) -T userprogs/hello.ld -o $@ $< -ffreestanding -O2 -nostdlib

$(BUILD)/hello_blob.o: $(BUILD)/hello.elf
	cd $(BUILD) && $(LD) -r -b binary -o hello_blob.o hello.elf

# --- M8: the ring-3 test program -----------------------------------------
# Same embedding trick, different program: usertest.c makes a syscall, then
# deliberately tries a direct hardware write to prove ring 3 can't.
$(BUILD)/usertest.o: userprogs/usertest.c | $(BUILD)
	$(CC) -c $< -o $@ -std=gnu99 -ffreestanding -O2 -Wall -Wextra

$(BUILD)/usertest.elf: $(BUILD)/usertest.o userprogs/usertest.ld
	$(CC) -T userprogs/usertest.ld -o $@ $< -ffreestanding -O2 -nostdlib

$(BUILD)/usertest_blob.o: $(BUILD)/usertest.elf
	cd $(BUILD) && $(LD) -r -b binary -o usertest_blob.o usertest.elf

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
