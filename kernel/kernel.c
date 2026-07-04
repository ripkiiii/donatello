/* kernel.c — the brain of Donatello. Orchestrates the boot sequence; each
 * subsystem (terminal, GDT, IDT, memory) lives in its own module. */

#include <stdint.h>
#include "terminal.h"
#include "gdt.h"
#include "idt.h"
#include "irq.h"
#include "keyboard.h"
#include "shell.h"
#include "multiboot.h"
#include "pmm.h"
#include "paging.h"
#include "heap.h"
#include "fs.h"

/* Set by the linker (linker.ld) at the very end of the kernel image. We take
 * its ADDRESS, not its value — the symbol marks a location, not a number. */
extern char kernel_end;

void kernel_main(uint32_t magic, multiboot_info_t* mbi) {
	term_init();

	/* Palette: white = primary/identity, grey = description,
	 * green = success confirmation ONLY, red = error. */
	term_setcolor(vga_color(VGA_WHITE, VGA_BLACK));
	term_write("Donatello\n");
	term_setcolor(vga_color(VGA_LIGHT_GREY, VGA_BLACK));
	term_write("An OS written from scratch. The brain is awake.\n");

	gdt_init();
	term_setcolor(vga_color(VGA_LIGHT_GREEN, VGA_BLACK));
	term_write("GDT installed.\n");

	idt_init();
	term_write("IDT installed.\n");

	pic_remap();       /* move hardware IRQs to 32..47 */
	keyboard_init();   /* register our IRQ1 handler */
	term_write("Keyboard live.\n");

	/* --- physical memory (M6a) ------------------------------------ */
	uint32_t mem_upper = 0;
	if (magic == MULTIBOOT_BOOTLOADER_MAGIC && (mbi->flags & MULTIBOOT_FLAG_MEM))
		mem_upper = mbi->mem_upper;

	if (mem_upper == 0) {
		term_setcolor(vga_color(VGA_LIGHT_RED, VGA_BLACK));
		term_write("No memory map from bootloader; assuming 32 MB.\n");
		mem_upper = 31744;   /* 32 MB total minus the low 1 MB, in KB */
	}

	pmm_init(mem_upper, (uint32_t)&kernel_end);

	term_setcolor(vga_color(VGA_LIGHT_GREY, VGA_BLACK));
	term_write("Memory: ");
	term_write_dec((mem_upper + 1024) / 1024);
	term_write(" MB, ");
	term_write_dec(pmm_total_frames());
	term_write(" frames (");
	term_write_dec(pmm_used_frames());
	term_write(" held by kernel).\n");
	term_setcolor(vga_color(VGA_LIGHT_GREEN, VGA_BLACK));
	term_write("PMM ready.\n");

	/* --- virtual memory (M6b) ------------------------------------- */
	paging_init();     /* identity-map RAM + turn the MMU on */
	term_setcolor(vga_color(VGA_LIGHT_GREEN, VGA_BLACK));
	term_write("MMU enabled.\n");

	/* --- heap (M6c) ------------------------------------------------ */
	kheap_init();      /* kmalloc/kfree on top of the PMM */

	/* --- filesystem (M9) -------------------------------------------- */
	fs_init();         /* read the file table off disk (sector 0) */
	term_setcolor(vga_color(VGA_LIGHT_GREEN, VGA_BLACK));
	term_write("Filesystem ready (");
	term_write_dec(fs_count());
	term_write(" file(s) on disk).\n");

	term_setcolor(vga_color(VGA_LIGHT_GREY, VGA_BLACK));
	shell_init();      /* banner + first prompt (M5) */

	asm volatile ("sti");            /* let hardware interrupts through */
	for (;;)
		asm volatile ("hlt");    /* sleep until the next interrupt */
}
