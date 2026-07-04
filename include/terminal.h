/* terminal.h — writing text to the VGA screen.
 *
 * Extracted from kernel.c so anything (including interrupt handlers) can
 * print, not just kernel_main. */
#ifndef TERMINAL_H
#define TERMINAL_H

#include <stdint.h>

enum vga_color {
	VGA_BLACK       = 0,
	VGA_GREEN       = 2,
	VGA_RED         = 4,
	VGA_LIGHT_GREY  = 7,
	VGA_LIGHT_GREEN = 10,
	VGA_LIGHT_RED   = 12,
	VGA_WHITE       = 15,
};

uint8_t vga_color(enum vga_color fg, enum vga_color bg);

void term_init(void);
void term_clear(void);
void term_setcolor(uint8_t color);
void term_write(const char* s);
void term_write_dec(uint32_t n);   /* print an unsigned number in decimal */
void term_write_hex(uint32_t n);   /* print a 32-bit value as 8 hex digits */

#endif
