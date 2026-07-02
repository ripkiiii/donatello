/* linker.ld — the map that stitches boot.s + kernel.c into one kernel image
 * and decides where each piece lands in memory. */

/* Execution starts at the _start label from boot.s. */
ENTRY(_start)

SECTIONS
{
	/* Load the kernel at the 1 MiB mark. The first megabyte of memory is
	 * cluttered with legacy junk (BIOS, VGA, etc.), so kernels live above it. */
	. = 1M;

	/* Code. The .multiboot header goes FIRST so the bootloader finds it
	 * within the first few KiB of the image. */
	.text BLOCK(4K) : ALIGN(4K)
	{
		*(.multiboot)
		*(.text)
	}

	/* Read-only data (string literals like "Hello from Donatello."). */
	.rodata BLOCK(4K) : ALIGN(4K)
	{
		*(.rodata)
	}

	/* Initialized read/write data. */
	.data BLOCK(4K) : ALIGN(4K)
	{
		*(.data)
	}

	/* Zero-initialized data — our stack from boot.s lives here. */
	.bss BLOCK(4K) : ALIGN(4K)
	{
		*(COMMON)
		*(.bss)
	}
}
