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
#include <stdint.h>
#include "shell.h"
#include "terminal.h"
#include "string.h"
#include "heap.h"
#include "elf.h"
#include "gdt.h"
#include "pmm.h"
#include "paging.h"
#include "usermode.h"
#include "ata.h"
#include "fs.h"

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
	term_write("  heaptest      alloc/free/reuse a few blocks (tests kmalloc)\n");
	term_write("  ls            list files on the disk\n");
	term_write("  run <name>    load a file from disk and execute it (ring 0)\n");
	term_write("  usermode      run 'usertest' in RING 3 (syscall + isolation test)\n");
	term_write("  diskread      read sector 0 off the ATA disk (tests the driver)\n");
}

/* M9 (9a): read the very first sector off disk and print it back as text,
 * so a human can visually confirm the bytes that came back are the exact
 * bytes the Makefile stamped into the test disk image — proof the driver
 * is talking to real (virtual) hardware, not just returning zeroed memory
 * that happens to look like success. */
static void cmd_diskread(void) {
	uint8_t buf[ATA_SECTOR_SIZE];

	if (!ata_read_sector(0, buf)) {
		term_setcolor(vga_color(VGA_LIGHT_RED, VGA_BLACK));
		term_write("diskread: ATA read error\n");
		term_setcolor(vga_color(VGA_WHITE, VGA_BLACK));
		return;
	}

	term_write("sector 0, first 32 bytes as text: \"");
	for (int i = 0; i < 32; i++) {
		char c = buf[i];
		char s[2] = { (c >= 32 && c < 127) ? c : '.', '\0' };
		term_write(s);
	}
	term_write("\"\n");
}

/* M9: list what's on disk, straight from the file table fs_init() already
 * read at boot — no extra disk access needed just to list names. */
static void cmd_ls(void) {
	int n = fs_count();
	if (n == 0) {
		term_write("(no files on disk)\n");
		return;
	}
	for (int i = 0; i < n; i++) {
		const fs_entry_t* e = fs_entry_at(i);
		term_write("  ");
		term_write(e->name);
		term_write("  (");
		term_write_dec(e->size_bytes);
		term_write(" bytes)\n");
	}
}

/* M9: find `name` on disk, read its bytes into fs_load()'s buffer, hand
 * that to the ELF loader (ring 0, same as M7's original behavior — just
 * sourced from disk instead of an embedded blob). If it's valid, elf_load()
 * jumps into it and never returns; everything after only runs on failure. */
static void cmd_run(const char* name) {
	if (!*name) {
		term_write("usage: run <name>  (try 'ls' to see what's on disk)\n");
		return;
	}

	uint32_t size;
	const uint8_t* data = fs_load(name, &size);
	if (!data)
		return;   /* fs_load already printed why */

	term_write("run: loaded ");
	term_write_dec(size);
	term_write(" bytes, parsing ELF...\n");

	elf_load(data);
	term_setcolor(vga_color(VGA_LIGHT_RED, VGA_BLACK));
	term_write("run: elf_load returned (it shouldn't have) -- load failed.\n");
	term_setcolor(vga_color(VGA_WHITE, VGA_BLACK));
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

/* Alloc three blocks, free the MIDDLE one, alloc a fourth — if the heap is
 * correct, the fourth allocation reuses exactly the address freed, proving
 * kmalloc actually searches the free list instead of just growing forever. */
static void cmd_heaptest(void) {
	term_write("a = kmalloc(64), b = kmalloc(64), c = kmalloc(64)\n");
	void* a = kmalloc(64);
	void* b = kmalloc(64);
	void* c = kmalloc(64);
	term_write("  a=0x"); term_write_hex((uint32_t)a); term_write("\n");
	term_write("  b=0x"); term_write_hex((uint32_t)b); term_write("\n");
	term_write("  c=0x"); term_write_hex((uint32_t)c); term_write("\n");

	term_write("kfree(b)\n");
	kfree(b);

	term_write("d = kmalloc(64) -- should reuse b's address:\n");
	void* d = kmalloc(64);
	term_write("  d=0x"); term_write_hex((uint32_t)d);
	term_setcolor(vga_color((d == b) ? VGA_LIGHT_GREEN : VGA_LIGHT_RED, VGA_BLACK));
	term_write((d == b) ? "  REUSED (correct)\n" : "  NOT reused (bug!)\n");
	term_setcolor(vga_color(VGA_WHITE, VGA_BLACK));

	kfree(a);
	kfree(c);
	kfree(d);
}

/* M8 (now M9-sourced): load the ring-3 test program off disk, grant it
 * (and only it) access to its own code/data and a dedicated stack, give
 * the CPU a safe kernel stack to use if it's interrupted while running,
 * then drop into ring 3. */
static void cmd_usermode(void) {
	uint32_t size;
	const uint8_t* data = fs_load("usertest", &size);
	if (!data)
		return;   /* fs_load already printed why */

	uint32_t range_start, range_end;
	uint32_t entry = elf_parse(data, &range_start, &range_end);
	if (!entry)
		return;   /* elf_parse already printed why */

	uint32_t user_stack_frame = pmm_alloc_frame();
	uint32_t kernel_stack_frame = pmm_alloc_frame();
	if (!user_stack_frame || !kernel_stack_frame) {
		term_setcolor(vga_color(VGA_LIGHT_RED, VGA_BLACK));
		term_write("usermode: out of physical memory\n");
		term_setcolor(vga_color(VGA_WHITE, VGA_BLACK));
		return;
	}
	uint32_t user_stack_top = user_stack_frame + FRAME_SIZE;

	/* Ring 3 may touch its own code/data and its own stack. Nothing else —
	 * VGA memory, the kernel, every other frame of RAM — stays
	 * supervisor-only, which is what the program's own second half tries
	 * to prove by deliberately violating it. */
	paging_set_user_range(range_start, range_end - range_start);
	paging_set_user_range(user_stack_frame, FRAME_SIZE);

	/* Must be set before the FIRST trip into ring 3 — the CPU consults
	 * this the instant any interrupt (the syscall, or the page fault
	 * we're about to provoke) fires while CPL is 3. */
	tss_set_kernel_stack(kernel_stack_frame + FRAME_SIZE);

	term_setcolor(vga_color(VGA_LIGHT_GREEN, VGA_BLACK));
	term_write("usermode: entering ring 3.\n");

	enter_usermode(entry, user_stack_top);
	/* Unreachable on success — enter_usermode() never returns. */
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
	else if (strcmp(cmd, "heaptest")  == 0) cmd_heaptest();
	else if (strcmp(cmd, "run")       == 0) cmd_run(arg);
	else if (strcmp(cmd, "usermode")  == 0) cmd_usermode();
	else if (strcmp(cmd, "diskread")  == 0) cmd_diskread();
	else if (strcmp(cmd, "ls")        == 0) cmd_ls();
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
