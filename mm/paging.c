/* paging.c — 2-level paging on i686, identity mapped.
 *
 * How address translation works once this is on:
 *
 *   virtual addr ─┬─ bits 31..22 (10) ─▶ index into the PAGE DIRECTORY
 *                 ├─ bits 21..12 (10) ─▶ index into a PAGE TABLE
 *                 └─ bits 11..0  (12) ─▶ offset inside the 4 KB frame
 *
 * Directory[1024] → each entry points at a Table[1024] → each entry points at
 * a physical frame. One table covers 4 MB. We fill them so virtual X maps to
 * physical X ("identity"), meaning nothing moves when we flip paging on — the
 * kernel keeps running at the same addresses, video memory stays at 0xB8000.
 *
 * The subtle part: we build these tables while paging is still OFF, so writes
 * go straight to physical RAM. Every table frame comes from the PMM out of low
 * memory, so once paging turns on, the tables map themselves. No crash. */

#include <stdint.h>
#include "paging.h"
#include "pmm.h"
#include "terminal.h"

#define PAGE_PRESENT 0x1        /* entry is valid              */
#define PAGE_RW      0x2        /* writable                    */
#define PAGE_USER    0x4        /* ring 3 may access this page */

#define ENTRIES      1024       /* entries per directory / table */
#define TABLE_SPAN   0x400000   /* one page table covers 4 MB    */

/* Physical address of our page directory (also what we load into CR3). Kept
 * as a file global so it lives for the whole kernel lifetime. */
static uint32_t* page_directory;

void paging_init(void) {
	uint32_t total_bytes = pmm_total_frames() * FRAME_SIZE;

	/* How many 4 MB page tables to cover all RAM (round up). */
	uint32_t num_tables = (total_bytes + TABLE_SPAN - 1) / TABLE_SPAN;

	/* Directory: 1024 entries, one frame, all "not present" to start. */
	page_directory = (uint32_t*)pmm_alloc_frame();
	for (uint32_t i = 0; i < ENTRIES; i++)
		page_directory[i] = 0;

	/* Fill one page table per 4 MB region, mapping each virtual page to the
	 * identical physical frame. Anything past the end of real RAM is left
	 * "not present" so a stray access there faults instead of reading junk. */
	for (uint32_t t = 0; t < num_tables; t++) {
		uint32_t* table = (uint32_t*)pmm_alloc_frame();
		for (uint32_t i = 0; i < ENTRIES; i++) {
			uint32_t phys = t * TABLE_SPAN + i * FRAME_SIZE;
			table[i] = (phys < total_bytes)
			         ? (phys | PAGE_PRESENT | PAGE_RW)
			         : 0;
		}
		page_directory[t] = (uint32_t)table | PAGE_PRESENT | PAGE_RW;
	}

	/* CR3 holds the physical address of the directory; the CPU starts its
	 * page-table walk from here. The "memory" clobber stops the compiler from
	 * reordering the table writes above to after paging is enabled. */
	asm volatile ("mov %0, %%cr3" :: "r"(page_directory) : "memory");

	/* Flip the master switch: CR0 bit 31 (PG) turns the MMU on. From the very
	 * next instruction, every address goes through the tables above. */
	uint32_t cr0;
	asm volatile ("mov %%cr0, %0" : "=r"(cr0));
	cr0 |= 0x80000000;
	asm volatile ("mov %0, %%cr0" :: "r"(cr0) : "memory");

	term_setcolor(vga_color(VGA_LIGHT_GREY, VGA_BLACK));
	term_write("Paging: identity-mapped ");
	term_write_dec(total_bytes / (1024 * 1024));
	term_write(" MB (");
	term_write_dec(num_tables + 1);
	term_write(" frames of tables).\n");
}

/* Mark [vaddr, vaddr+size) as accessible from ring 3 (M8). Everything else
 * — the kernel, VGA memory, all other RAM — stays supervisor-only, so a
 * ring 3 program can only touch the specific pages we hand it here.
 *
 * The U/S bit must be set on BOTH levels of the walk: the directory entry
 * AND the table entry. The CPU effectively ANDs permissions along the
 * path — if either level says "supervisor only", the access is denied
 * regardless of what the other level says. Setting it on the directory
 * entry is safe even though that entry covers a whole 4 MB table shared
 * with other (still kernel-only) pages: real access control stays
 * per-page at the table level; the directory bit is just a prerequisite. */
void paging_set_user_range(uint32_t vaddr, uint32_t size) {
	uint32_t start = vaddr & ~0xFFF;
	uint32_t end   = (vaddr + size + 0xFFF) & ~0xFFF;

	for (uint32_t addr = start; addr < end; addr += FRAME_SIZE) {
		uint32_t dir_idx = addr >> 22;
		uint32_t tbl_idx = (addr >> 12) & 0x3FF;
		uint32_t* table  = (uint32_t*)(page_directory[dir_idx] & ~0xFFF);

		page_directory[dir_idx] |= PAGE_USER;
		table[tbl_idx]          |= PAGE_USER;
	}
}
