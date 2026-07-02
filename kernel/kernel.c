/* kernel.c — the brain of Donatello. Just the boot sequence now; the
 * terminal, GDT and IDT each live in their own module. */

#include "terminal.h"
#include "gdt.h"
#include "idt.h"
#include "irq.h"
#include "keyboard.h"

void kernel_main(void) {
	term_init();

	term_setcolor(vga_color(VGA_LIGHT_GREEN, VGA_BLACK));
	term_write("Hello from Donatello.\n");
	term_setcolor(vga_color(VGA_LIGHT_GREY, VGA_BLACK));
	term_write("An OS written from scratch. The brain is awake.\n");

	gdt_init();
	term_setcolor(vga_color(VGA_LIGHT_GREEN, VGA_BLACK));
	term_write("GDT installed. Memory is ours now.\n");

	idt_init();
	term_setcolor(vga_color(VGA_LIGHT_GREY, VGA_BLACK));
	term_write("IDT installed.\n");

	pic_remap();       /* move hardware IRQs to 32..47 */
	keyboard_init();   /* register our IRQ1 handler */

	term_setcolor(vga_color(VGA_LIGHT_GREEN, VGA_BLACK));
	term_write("Keyboard live. Type something:\n\n");
	term_setcolor(vga_color(VGA_WHITE, VGA_BLACK));

	asm volatile ("sti");            /* let hardware interrupts through */
	for (;;)
		asm volatile ("hlt");    /* sleep until the next interrupt */
}
