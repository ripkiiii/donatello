/* gdt.h — set up our own Global Descriptor Table. */
#ifndef GDT_H
#define GDT_H

#include <stdint.h>

/* Segment selectors (index << 3 | RPL). Ring 3 code jumps to KERNEL_CS/DS
 * ONLY through a controlled gate (syscall) — its own CS/DS/SS are these
 * USER_* selectors, whose descriptors carry DPL=3. */
#define KERNEL_CS 0x08
#define KERNEL_DS 0x10
#define USER_CS   0x1B   /* (3 << 3) | 3 — RPL 3 baked into the selector  */
#define USER_DS   0x23   /* (4 << 3) | 3                                 */

/* Build a flat GDT (null, kernel code/data, user code/data, TSS) and load
 * it, including the TSS via `ltr`. */
void gdt_init(void);

/* Tell the CPU which kernel stack to switch to whenever a ring 3 -> ring 0
 * transition happens (any interrupt firing while we're in user mode). Call
 * this with a fresh stack top BEFORE the first trip into ring 3. */
void tss_set_kernel_stack(uint32_t esp0);

#endif
