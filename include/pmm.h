/* pmm.h — the Physical Memory Manager. Hands out raw 4 KB chunks of physical
 * RAM ("frames"). This is the floor everything else stands on: paging needs
 * frames for its page tables, the heap needs frames for its pages. */
#ifndef PMM_H
#define PMM_H

#include <stdint.h>

#define FRAME_SIZE 4096          /* one physical frame = 4 KB (a "page")   */

/* Set up the allocator. mem_upper_kb comes from multiboot (RAM above 1 MB);
 * kernel_end is where our kernel image stops, so we never hand out memory the
 * kernel itself is sitting on. */
void     pmm_init(uint32_t mem_upper_kb, uint32_t kernel_end);

/* Grab one free frame. Returns its physical address, or 0 if RAM is full. */
uint32_t pmm_alloc_frame(void);

/* Give a frame back so it can be reused. */
void     pmm_free_frame(uint32_t addr);

uint32_t pmm_total_frames(void);
uint32_t pmm_used_frames(void);

#endif
