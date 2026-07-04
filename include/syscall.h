/* syscall.h — the one door ring 3 code has into the kernel.
 *
 * A user program can't call term_write() directly (it's kernel code,
 * mapped supervisor-only) — instead it loads a syscall number into EAX,
 * puts arguments in the other registers, and executes `int $0x80`. isr128
 * (isr.s) catches that, builds a registers_t exactly like every other
 * interrupt, and hands it here. */
#ifndef SYSCALL_H
#define SYSCALL_H

#include "idt.h"

#define SYS_WRITE 1   /* ebx = pointer to a NUL-terminated string */

void syscall_handler(registers_t* regs);

#endif
