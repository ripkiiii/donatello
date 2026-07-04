/* heap.h — kmalloc/kfree: general-purpose allocation on top of the PMM.
 *
 * PMM only hands out whole 4 KB frames — fine for page tables, useless for a
 * 40-byte struct. The heap subdivides frames into smaller chunks and tracks
 * which ones are free, so kernel code can finally ask for "just enough". */
#ifndef HEAP_H
#define HEAP_H

#include <stddef.h>

void  kheap_init(void);           /* claim the first frame to bootstrap the heap */
void* kmalloc(size_t size);       /* allocate; NULL if physical memory is exhausted */
void  kfree(void* ptr);           /* return a block; coalesces with its free neighbor */

#endif
