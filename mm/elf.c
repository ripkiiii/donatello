/* elf.c — the ELF32 loader: turn a binary blob into a running program.
 *
 * ELF (Executable and Linkable Format) is what Linux, our cross-compiler's
 * output, and this program itself are all shaped like. Two headers matter
 * for running one:
 *
 *   ELF header    — "is this really an ELF file, and where do I start?"
 *   Program header(s) — "what bytes go at what address, before I jump?"
 *
 * There's usually more (section headers, symbol tables, relocations) but
 * none of that is needed just to RUN something — only to link or debug it.
 * A loader's job is narrow: copy the LOAD segments into place, then jump to
 * the entry point. That's it. */

#include <stdint.h>
#include "elf.h"
#include "terminal.h"
#include "string.h"

/* --- the two structs the spec actually requires us to read -------------- */

typedef struct {
	uint8_t  e_ident[16];  /* magic + class + endianness + ABI info */
	uint16_t e_type;       /* 2 = ET_EXEC (a plain runnable executable) */
	uint16_t e_machine;    /* 3 = EM_386 (x86)                          */
	uint32_t e_version;
	uint32_t e_entry;      /* virtual address to jump to after loading  */
	uint32_t e_phoff;      /* file offset of the program header table   */
	uint32_t e_shoff;
	uint32_t e_flags;
	uint16_t e_ehsize;
	uint16_t e_phentsize;  /* size of ONE program header entry          */
	uint16_t e_phnum;      /* how many program header entries there are */
	uint16_t e_shentsize;
	uint16_t e_shnum;
	uint16_t e_shstrndx;
} __attribute__((packed)) elf32_ehdr_t;

typedef struct {
	uint32_t p_type;    /* 1 = PT_LOAD (a segment we actually need to copy) */
	uint32_t p_offset;  /* where these bytes live IN THE FILE               */
	uint32_t p_vaddr;   /* where they need to end up IN MEMORY              */
	uint32_t p_paddr;
	uint32_t p_filesz;  /* bytes present in the file                        */
	uint32_t p_memsz;   /* bytes it occupies in memory (>= filesz; the      */
	                    /* difference is .bss — present but zeroed, saving  */
	                    /* space in the file for data that starts at zero) */
	uint32_t p_flags;
	uint32_t p_align;
} __attribute__((packed)) elf32_phdr_t;

#define PT_LOAD     1
#define EM_386      3
#define ET_EXEC     2
#define ELFCLASS32  1
#define ELFDATA2LSB 1

static int elf_valid(const elf32_ehdr_t* eh) {
	if (eh->e_ident[0] != 0x7F || eh->e_ident[1] != 'E' ||
	    eh->e_ident[2] != 'L'  || eh->e_ident[3] != 'F') {
		term_write("elf: not an ELF file (bad magic)\n");
		return 0;
	}
	if (eh->e_ident[4] != ELFCLASS32) {
		term_write("elf: not 32-bit\n");
		return 0;
	}
	if (eh->e_ident[5] != ELFDATA2LSB) {
		term_write("elf: not little-endian\n");
		return 0;
	}
	if (eh->e_type != ET_EXEC) {
		term_write("elf: not an executable (ET_EXEC)\n");
		return 0;
	}
	if (eh->e_machine != EM_386) {
		term_write("elf: wrong architecture (not i386)\n");
		return 0;
	}
	return 1;
}

int elf_load(const uint8_t* data) {
	const elf32_ehdr_t* eh = (const elf32_ehdr_t*)data;

	if (!elf_valid(eh))
		return 0;

	term_setcolor(vga_color(VGA_LIGHT_GREY, VGA_BLACK));
	term_write("elf: entry=0x");
	term_write_hex(eh->e_entry);
	term_write(", ");
	term_write_dec(eh->e_phnum);
	term_write(" program header(s)\n");

	/* Copy every LOAD segment to the address it was linked for. Because
	 * paging identity-maps all RAM, "the address it was linked for" and
	 * "a real, writable memory location" are the same number — no new
	 * page-table entries needed at this stage (that changes once user
	 * space gets its own address layout in M8). */
	const elf32_phdr_t* ph =
	    (const elf32_phdr_t*)(data + eh->e_phoff);

	for (int i = 0; i < eh->e_phnum; i++) {
		if (ph[i].p_type != PT_LOAD)
			continue;

		uint8_t* dest = (uint8_t*)ph[i].p_vaddr;
		const uint8_t* src = data + ph[i].p_offset;

		memcpy(dest, src, ph[i].p_filesz);

		/* memsz can exceed filesz — that gap is .bss (zeroed globals):
		 * present in memory, but not worth storing zero bytes in the
		 * file. Zero it ourselves since nothing else will. */
		if (ph[i].p_memsz > ph[i].p_filesz)
			memset(dest + ph[i].p_filesz, 0,
			       ph[i].p_memsz - ph[i].p_filesz);
	}

	term_setcolor(vga_color(VGA_LIGHT_GREEN, VGA_BLACK));
	term_write("elf: jumping to entry point.\n");

	/* No process table, no return address to come back to — we ARE this
	 * program now. A successful load doesn't return; if execution ever
	 * gets past this call, something has gone very wrong. */
	void (*entry)(void) = (void (*)(void))eh->e_entry;
	entry();

	return 1;   /* unreachable in practice; kept honest for the prototype */
}
