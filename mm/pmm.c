/* pmm.c — physical frame allocator, bitmap flavor.
 *
 * Picture all of physical RAM sliced into 4 KB frames. We keep one bit per
 * frame: 0 = free, 1 = used. Allocating = find a 0 bit, flip it to 1, return
 * that frame's address. Simple, and the whole map for 512 MB is just 16 KB. */

#include <stddef.h>
#include "pmm.h"

/* Support up to 512 MB of RAM. 512 MB / 4 KB = 131072 frames. At 32 frames
 * per uint32_t that's a 16 KB bitmap living in .bss. */
#define MAX_FRAMES 131072
#define BITMAP_LEN (MAX_FRAMES / 32)

static uint32_t frame_bitmap[BITMAP_LEN];
static uint32_t total_frames;
static uint32_t used_frames;

/* --- bit helpers: frame index <-> bit in the map -------------------- */

static void mark_used(uint32_t i) { frame_bitmap[i / 32] |=  (1u << (i % 32)); }
static void mark_free(uint32_t i) { frame_bitmap[i / 32] &= ~(1u << (i % 32)); }
static int  is_used  (uint32_t i) { return frame_bitmap[i / 32] &   (1u << (i % 32)); }

/* --- API ------------------------------------------------------------ */

void pmm_init(uint32_t mem_upper_kb, uint32_t kernel_end) {
	/* mem_upper is RAM above the 1 MB line; add that 1 MB back for the total. */
	uint32_t total_bytes = (mem_upper_kb + 1024) * 1024;
	total_frames = total_bytes / FRAME_SIZE;
	if (total_frames > MAX_FRAMES)
		total_frames = MAX_FRAMES;

	/* Start pessimistic: everything used. Then hand back the frames we KNOW
	 * are safe — the ones sitting past the end of the kernel image. Anything
	 * below kernel_end (BIOS, our code/data/stack, this very bitmap) stays
	 * locked so we never allocate memory we're standing on. */
	for (uint32_t i = 0; i < BITMAP_LEN; i++)
		frame_bitmap[i] = 0xFFFFFFFF;
	used_frames = total_frames;

	uint32_t first_free = (kernel_end + FRAME_SIZE - 1) / FRAME_SIZE;  /* round up */
	for (uint32_t i = first_free; i < total_frames; i++)
		pmm_free_frame(i * FRAME_SIZE);
}

uint32_t pmm_alloc_frame(void) {
	for (uint32_t i = 0; i < total_frames; i++) {
		if (!is_used(i)) {
			mark_used(i);
			used_frames++;
			return i * FRAME_SIZE;
		}
	}
	return 0;   /* out of physical memory */
}

void pmm_free_frame(uint32_t addr) {
	uint32_t i = addr / FRAME_SIZE;
	if (i >= total_frames)
		return;
	if (!is_used(i))
		return;                 /* already free — don't double-count */
	mark_free(i);
	used_frames--;
}

uint32_t pmm_total_frames(void) { return total_frames; }
uint32_t pmm_used_frames(void)  { return used_frames;  }
