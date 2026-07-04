/* hello.c — the first program that ISN'T the kernel itself.
 *
 * Deliberately dumb and self-contained: writes straight to VGA memory at
 * 0xB8000, bypassing terminal.c entirely. The point of M7 isn't THIS
 * program — it's proving the loader can take a genuinely separate ELF
 * binary, put it in memory, and jump to it. A program that reused kernel
 * helper functions would leave that unproven (did the loader really work,
 * or did it just call back into the kernel we were already running?).
 *
 * No syscalls exist yet (that's M8) so this can't ask the kernel to print
 * for it — writing hardware memory directly is the only thing a freestanding
 * program can do on its own at this stage. */

#include <stdint.h>

void _start(void) {
	volatile uint16_t* vga = (volatile uint16_t*)0xB8000;
	const char* msg = "Hello from a LOADED program! (not the kernel talking)";

	/* Second line of the screen (row 1), so it doesn't fight the kernel's
	 * own output on row 0. Color 0x1F = white text, blue background — a
	 * different palette on purpose, so it's visually obvious this text
	 * came from somewhere other than the kernel's usual grey-on-black. */
	for (int i = 0; msg[i]; i++)
		vga[80 + i] = (uint16_t)msg[i] | (0x1F << 8);

	/* No OS to return to yet (no syscall to exit through) — just halt. */
	for (;;)
		asm volatile ("hlt");
}
