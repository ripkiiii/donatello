/* syscall.c — dispatch table for the one door ring 3 has into the kernel.
 *
 * Every syscall arrives the same way: regs->eax is the syscall number,
 * regs->ebx/ecx/edx/... are arguments (this mirrors the classic Linux
 * i386 `int $0x80` calling convention, though our numbers don't match
 * Linux's — there's no reason they'd need to). */

#include "syscall.h"
#include "terminal.h"

/* HONEST LIMITATION: we trust `regs->ebx` as a valid, NUL-terminated
 * string pointer with zero validation. A real kernel must check that a
 * user-supplied pointer actually falls within memory THAT USER PROGRAM is
 * allowed to read (not kernel memory, not another process's memory) before
 * ever dereferencing it — a huge fraction of real-world privilege
 * escalation bugs are exactly this check missing or done wrong. Skipped
 * here to keep the first syscall's plumbing (the interrupt path, the
 * ring transition, the dispatch) the whole point of this milestone;
 * validating user pointers properly is its own piece of work for later. */
void syscall_handler(registers_t* regs) {
	switch (regs->eax) {
	case SYS_WRITE:
		term_write((const char*)regs->ebx);
		regs->eax = 0;   /* success — becomes the real EAX on return */
		break;

	default:
		term_setcolor(vga_color(VGA_LIGHT_RED, VGA_BLACK));
		term_write("syscall: unknown number ");
		term_write_dec(regs->eax);
		term_write("\n");
		term_setcolor(vga_color(VGA_WHITE, VGA_BLACK));
		regs->eax = (uint32_t)-1;
	}
}
