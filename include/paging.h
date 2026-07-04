/* paging.h — turn on the MMU (Memory Management Unit).
 *
 * Once paging is on, every address the CPU touches is VIRTUAL: the MMU walks
 * our page tables to find the real physical frame. We set up an "identity
 * map" first (virtual X -> physical X for all real RAM) so that flipping the
 * switch changes nothing the CPU can see — the safest possible first step. */
#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>

/* Build the identity map for all detected RAM, then enable the MMU. */
void paging_init(void);

/* Mark a range as ring-3 accessible (M8). Everything else stays
 * supervisor-only — this is what makes user-mode isolation real rather
 * than just a CPU privilege check. */
void paging_set_user_range(uint32_t vaddr, uint32_t size);

#endif
