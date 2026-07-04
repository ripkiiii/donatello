/* elf.h — parse and load a 32-bit ELF executable already sitting in memory.
 *
 * "Already in memory" is the key phrase: the parsing/loading logic here
 * doesn't care WHERE the bytes came from — M7/M8 got them from a blob
 * embedded into the kernel image at build time; M9's fs_load() reads them
 * off the (virtual) disk into a buffer instead. Same elf_parse()/elf_load()
 * either way — exactly the "small change, not a rewrite" this header
 * predicted back in M7. */
#ifndef ELF_H
#define ELF_H

#include <stdint.h>

/* Validates and copies an ELF32 executable's PT_LOAD segments into place,
 * WITHOUT jumping to it. Returns the entry point address (0 = invalid
 * file — details already printed). If out_start/out_end are non-NULL,
 * they're filled with the [lowest vaddr, highest vaddr+size) range the
 * segments occupy — callers that need to grant ring-3 access to exactly
 * that memory (M8) use this instead of elf_load(). */
uint32_t elf_parse(const uint8_t* data, uint32_t* out_start, uint32_t* out_end);

/* Returns 1 and calls into the program's entry point if `data` is a valid
 * ELF32 executable this loader understands. Returns 0 (and prints why) on
 * anything it can't or won't run. This function only returns at all if the
 * ELF failed to load — a successful load jumps away and never comes back
 * (there's no process to return TO yet; that needs a scheduler).
 *
 * Runs the entry point in ring 0 — no isolation. See M8's `enter_usermode`
 * path (via elf_parse()) for running one in ring 3 instead. */
int elf_load(const uint8_t* data);

#endif
