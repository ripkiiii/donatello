/* paging.h — turn on the MMU (Memory Management Unit).
 *
 * Once paging is on, every address the CPU touches is VIRTUAL: the MMU walks
 * our page tables to find the real physical frame. We set up an "identity
 * map" first (virtual X -> physical X for all real RAM) so that flipping the
 * switch changes nothing the CPU can see — the safest possible first step. */
#ifndef PAGING_H
#define PAGING_H

/* Build the identity map for all detected RAM, then enable the MMU. */
void paging_init(void);

#endif
