/* io.h — talk to hardware over the CPU's separate I/O port space.
 *
 * Devices like the PIC and keyboard aren't in normal memory; they live behind
 * numbered "ports" you reach with the in/out instructions. */
#ifndef IO_H
#define IO_H

#include <stdint.h>

static inline void outb(uint16_t port, uint8_t val) {
	asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
	uint8_t ret;
	asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
	return ret;
}

/* Write to an unused port to burn a tiny, known delay — old hardware (the
 * PIC) needs a moment between commands. */
static inline void io_wait(void) {
	outb(0x80, 0);
}

#endif
