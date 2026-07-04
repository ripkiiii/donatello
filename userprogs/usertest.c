/* usertest.c — M8: the first program that runs in RING 3.
 *
 * Two things happen here, in order, each proving one half of what "user
 * mode" actually means:
 *
 *   1. A syscall (int $0x80) asks the KERNEL to print a message. This is
 *      the only way this program can affect the outside world at all —
 *      it has no direct access to VGA memory or any other hardware,
 *      unlike hello.c (M7), which ran in ring 0 and could poke 0xB8000
 *      freely.
 *
 *   2. It then deliberately tries to write to 0xB8000 (VGA memory)
 *      DIRECTLY, bypassing the syscall entirely. This SHOULD fail — the
 *      CPU should refuse the write and raise a page fault, because that
 *      page was never marked ring-3-accessible (only this program's own
 *      code/data/stack were). If the syscall message appears on screen
 *      but the "should have faulted" line never does, isolation is
 *      working exactly as intended. */

#include <stdint.h>

static inline int sys_write(const char* s) {
	int ret;
	/* "a"(1) as an INPUT and "=a"(ret) as the OUTPUT on the same register
	 * is the standard syscall-wrapper idiom: the compiler loads 1 into
	 * EAX before `int $0x80`; after it returns, EAX holds whatever the
	 * kernel's syscall_handler set regs->eax to, which becomes `ret`. */
	asm volatile ("int $0x80" : "=a"(ret) : "a"(1), "b"(s) : "memory");
	return ret;
}

void _start(void) {
	sys_write("Hello from ring 3! (this line came through a syscall)\n");

	/* Deliberate isolation test. Note: no `hlt` anywhere in this file —
	 * HLT is a privileged instruction, and ring 3 code executing it would
	 * itself trigger a general protection fault (vector 13) before we
	 * even get to test the thing we actually want to test. */
	volatile uint16_t* vga = (volatile uint16_t*)0xB8000;
	*vga = 'X' | (0x4F << 8);

	/* Unreachable if isolation works — the line above should have faulted. */
	sys_write("BUG: direct VGA write should have faulted before this.\n");
	for (;;)
		;
}
