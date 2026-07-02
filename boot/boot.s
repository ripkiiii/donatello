# boot.s — the very first code that runs in Donatello.
#
# Its whole job: make the machine recognize us as a kernel (Multiboot
# header), set up a stack so C can run, then hand control to kernel_main.

# --- Multiboot header constants -------------------------------------------
# GRUB/QEMU scan the first 8 KiB of the kernel for this magic header. If
# they find it, they know "this is a kernel I can boot" and jump to us.
.set ALIGN,    1<<0             # align loaded modules on page boundaries
.set MEMINFO,  1<<1             # ask the bootloader for a memory map
.set FLAGS,    ALIGN | MEMINFO  # the Multiboot flag field
.set MAGIC,    0x1BADB002       # the magic number that marks a Multiboot header
.set CHECKSUM, -(MAGIC + FLAGS) # MAGIC + FLAGS + CHECKSUM must sum to zero

# Put the header in its own section so the linker can place it right at the
# front of the binary (where the bootloader looks for it).
.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

# --- The stack ------------------------------------------------------------
# When the machine boots there is no stack yet, and C literally cannot run
# without one (function calls, local variables all live on the stack). So we
# carve out 16 KiB of space here and point the stack pointer at it.
.section .bss
.align 16
stack_bottom:
.skip 16384 # 16 KiB
stack_top:

# --- Entry point ----------------------------------------------------------
.section .text
.global _start
.type _start, @function
_start:
	# The stack grows downward, so we start the stack pointer at the TOP.
	mov $stack_top, %esp

	# The stage is set. Jump into C.
	call kernel_main

	# kernel_main should never return. If it does, freeze the CPU cleanly:
	cli          # disable interrupts
1:	hlt          # halt until an interrupt (there are none) — i.e. sleep
	jmp 1b       # if we ever wake, halt again. Infinite safe loop.

.size _start, . - _start
