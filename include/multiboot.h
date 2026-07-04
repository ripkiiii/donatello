/* multiboot.h — the handshake the bootloader (QEMU/GRUB) fills in for us.
 *
 * When a Multiboot loader jumps to our kernel it leaves two things behind:
 *   eax = a magic number proving we were loaded by a Multiboot loader
 *   ebx = a pointer to this struct, describing the machine (how much RAM, etc.)
 *
 * boot.s pushes both as arguments to kernel_main. We only spell out the early
 * fields we actually read — the struct is bigger, but C doesn't care as long
 * as our offsets match the spec. */
#ifndef MULTIBOOT_H
#define MULTIBOOT_H

#include <stdint.h>

/* eax holds this after a Multiboot boot. Note: NOT the same as the 0x1BADB002
 * header magic in boot.s — that one marks US to the loader; this one marks the
 * LOADER to us. */
#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002

/* flags bit 0: mem_lower / mem_upper are valid. */
#define MULTIBOOT_FLAG_MEM 0x001

typedef struct {
	uint32_t flags;
	uint32_t mem_lower;   /* KB of usable RAM below 1 MB */
	uint32_t mem_upper;   /* KB of usable RAM above 1 MB (this is our total) */
	uint32_t boot_device;
	uint32_t cmdline;
	uint32_t mods_count;
	uint32_t mods_addr;
	uint32_t syms[4];
	uint32_t mmap_length;
	uint32_t mmap_addr;
	/* ...more fields exist; we stop where we stop reading. */
} multiboot_info_t;

#endif
