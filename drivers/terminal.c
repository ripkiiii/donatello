/* terminal.c — the VGA text-mode terminal.
 *
 * The screen is an 80x25 grid at physical address 0xB8000. Each cell is two
 * bytes: [ ASCII character | color attribute ]. Write those and the glyph
 * appears instantly. */

#include <stddef.h>
#include "terminal.h"
#include "io.h"

#define VGA_WIDTH   80
#define VGA_HEIGHT  25
#define VGA_MEMORY  ((volatile uint16_t*)0xB8000)

static size_t   term_row;
static size_t   term_col;
static uint8_t  term_color;

uint8_t vga_color(enum vga_color fg, enum vga_color bg) {
	return fg | (bg << 4);
}

static uint16_t vga_entry(char c, uint8_t color) {
	return (uint16_t)c | ((uint16_t)color << 8);
}

void term_setcolor(uint8_t color) {
	term_color = color;
}

/* Move the blinking hardware cursor to our current row/column by writing the
 * cell index into the VGA CRTC registers (ports 0x3D4 index / 0x3D5 data). */
static void term_update_cursor(void) {
	uint16_t pos = term_row * VGA_WIDTH + term_col;
	outb(0x3D4, 14);                    /* cursor location high byte */
	outb(0x3D5, (pos >> 8) & 0xFF);
	outb(0x3D4, 15);                    /* cursor location low byte  */
	outb(0x3D5, pos & 0xFF);
}

void term_init(void) {
	term_row   = 0;
	term_col   = 0;
	term_color = vga_color(VGA_LIGHT_GREY, VGA_BLACK);
	for (size_t y = 0; y < VGA_HEIGHT; y++)
		for (size_t x = 0; x < VGA_WIDTH; x++)
			VGA_MEMORY[y * VGA_WIDTH + x] = vga_entry(' ', term_color);
	term_update_cursor();
}

/* Wipe the screen and go home. Same as init minus resetting the color. */
void term_clear(void) {
	for (size_t y = 0; y < VGA_HEIGHT; y++)
		for (size_t x = 0; x < VGA_WIDTH; x++)
			VGA_MEMORY[y * VGA_WIDTH + x] = vga_entry(' ', term_color);
	term_row = 0;
	term_col = 0;
	term_update_cursor();
}

/* When the cursor falls off the bottom, shift every row up by one and blank
 * the last row. VGA has no scroll command — we move the bytes ourselves. */
static void term_scroll(void) {
	for (size_t y = 1; y < VGA_HEIGHT; y++)
		for (size_t x = 0; x < VGA_WIDTH; x++)
			VGA_MEMORY[(y - 1) * VGA_WIDTH + x] =
			    VGA_MEMORY[y * VGA_WIDTH + x];
	for (size_t x = 0; x < VGA_WIDTH; x++)
		VGA_MEMORY[(VGA_HEIGHT - 1) * VGA_WIDTH + x] =
		    vga_entry(' ', term_color);
	term_row = VGA_HEIGHT - 1;
}

static void term_putchar(char c) {
	if (c == '\n') {
		term_col = 0;
		term_row++;
	} else if (c == '\b') {
		/* Step back one cell and erase what was there. */
		if (term_col > 0) {
			term_col--;
		} else if (term_row > 0) {
			term_row--;
			term_col = VGA_WIDTH - 1;
		}
		VGA_MEMORY[term_row * VGA_WIDTH + term_col] = vga_entry(' ', term_color);
	} else {
		VGA_MEMORY[term_row * VGA_WIDTH + term_col] = vga_entry(c, term_color);
		if (++term_col == VGA_WIDTH) {
			term_col = 0;
			term_row++;
		}
	}
	if (term_row == VGA_HEIGHT)
		term_scroll();
	term_update_cursor();
}

void term_write(const char* s) {
	for (size_t i = 0; s[i]; i++)
		term_putchar(s[i]);
}

void term_write_dec(uint32_t n) {
	if (n == 0) {
		term_putchar('0');
		return;
	}
	char buf[11];
	int i = 0;
	while (n > 0) {
		buf[i++] = '0' + (n % 10);
		n /= 10;
	}
	while (i-- > 0)
		term_putchar(buf[i]);
}
