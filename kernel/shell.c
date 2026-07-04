/* shell.c — the Donatello shell (M5). A REPL living inside the kernel:
 * collect a line (line discipline), split it into command + argument,
 * dispatch to a builtin.
 *
 * Honest note: this runs in IRQ context — shell_input() is called straight
 * from the keyboard interrupt handler. Real kernels defer this work to a
 * ring buffer drained outside the interrupt ("bottom half"). At our size
 * commands finish in microseconds, so simple wins. Revisit when a command
 * ever becomes slow. */

#include <stddef.h>
#include "shell.h"
#include "terminal.h"
#include "string.h"

#define LINE_MAX 128

static char   line[LINE_MAX];
static size_t line_len;

static void prompt(void) {
	/* Prompt shows on every line, so it stays neutral (grey) — green is
	 * reserved for success only. Input echoes white. */
	term_setcolor(vga_color(VGA_LIGHT_GREY, VGA_BLACK));
	term_write("donatello> ");
	term_setcolor(vga_color(VGA_WHITE, VGA_BLACK));
}

/* --- builtins ------------------------------------------------------- */

static void cmd_help(void) {
	term_write("builtins:\n");
	term_write("  help          show this list\n");
	term_write("  clear         wipe the screen\n");
	term_write("  echo <text>   print text back\n");
	term_write("  about         what is this OS\n");
	term_write("  pagefault     touch unmapped memory (tests the MMU)\n");
}

/* Deliberately read from an address past the identity map. The MMU has no
 * translation for it, so the CPU raises a page fault (#14) — caught and
 * reported by isr_handler instead of silently rebooting the machine. */
static void cmd_pagefault(void) {
	volatile uint32_t* bad = (uint32_t*)0x40000000;   /* 1 GB — not mapped */
	term_write("reading 0x40000000...\n");
	uint32_t x = *bad;
	(void)x;   /* never reached — the read above faults */
}

static void cmd_about(void) {
	term_write("Donatello - an OS from scratch (i686). boot.s -> GDT -> IDT\n");
	term_write("-> IRQ -> keyboard -> this shell. No libc, no Linux, all ours.\n");
}

/* --- dispatch ------------------------------------------------------- */

static void run_line(void) {
	/* skip leading spaces */
	char* cmd = line;
	while (*cmd == ' ')
		cmd++;

	if (*cmd == '\0')
		return;                          /* empty line: just reprompt */

	/* split "cmd arg..." at the first space; arg may be empty */
	char* arg = cmd;
	while (*arg && *arg != ' ')
		arg++;
	if (*arg == ' ') {
		*arg = '\0';                     /* terminate the command word */
		arg++;
		while (*arg == ' ')
			arg++;
	}

	if      (strcmp(cmd, "help")      == 0) cmd_help();
	else if (strcmp(cmd, "clear")     == 0) term_clear();
	else if (strcmp(cmd, "about")     == 0) cmd_about();
	else if (strcmp(cmd, "pagefault") == 0) cmd_pagefault();
	else if (strcmp(cmd, "echo")      == 0) {
		term_write(arg);
		term_write("\n");
	} else {
		term_setcolor(vga_color(VGA_LIGHT_RED, VGA_BLACK));
		term_write(cmd);
		term_write(": unknown command (try 'help')\n");
		term_setcolor(vga_color(VGA_WHITE, VGA_BLACK));
	}
}

/* --- input ---------------------------------------------------------- */

void shell_input(char c) {
	if (c == '\n') {
		term_write("\n");
		line[line_len] = '\0';
		run_line();
		line_len = 0;
		prompt();
	} else if (c == '\b') {
		if (line_len > 0) {              /* never erase the prompt */
			line_len--;
			term_write("\b");
		}
	} else if (line_len < LINE_MAX - 1) {
		line[line_len++] = c;
		char s[2] = { c, '\0' };
		term_write(s);                   /* echo */
	}
	/* line full: silently drop extra characters until Enter */
}

void shell_init(void) {
	line_len = 0;
	term_write("Type 'help' to see what I can do.\n\n");
	prompt();
}
