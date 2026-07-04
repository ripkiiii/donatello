/* keyboard.c — the IRQ1 handler. Reads a scancode from the keyboard, maps it
 * to an ASCII character, and hands it to the shell. The shell owns echoing
 * now — the driver just turns hardware bytes into characters. */

#include <stdint.h>
#include "keyboard.h"
#include "irq.h"
#include "io.h"
#include "shell.h"

#define KEYBOARD_DATA   0x60   /* read the scancode from here            */
#define KEYBOARD_STATUS 0x64   /* bit 0 set = a byte is waiting in 0x60  */

/* US QWERTY, scancode set 1. Index = scancode; value = ASCII (0 = ignore,
 * e.g. shift/ctrl/alt). Entries past space are left zero. */
static const char keymap[128] = {
	0,   27,  '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
	'\t','q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
	0,   'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'','`',
	0,   '\\','z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',
	0,   '*',
	0,   ' ',
};

static void keyboard_callback(registers_t* regs) {
	(void)regs;

	/* Drain every byte the controller has queued. If we only read one per
	 * interrupt, a burst of keys fills its buffer, OBF stays set, and it
	 * stops raising IRQs — the keyboard "freezes" after a few presses. */
	while (inb(KEYBOARD_STATUS) & 0x01) {
		uint8_t scancode = inb(KEYBOARD_DATA);

		/* Top bit set = a key was released (break code). Ignore for now. */
		if (scancode & 0x80)
			continue;

		char c = keymap[scancode];
		if (c)
			shell_input(c);
	}
}

void keyboard_init(void) {
	irq_install_handler(1, keyboard_callback);   /* keyboard is IRQ line 1 */
}
