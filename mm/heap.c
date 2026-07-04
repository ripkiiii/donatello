/* heap.c — a free-list allocator built on top of the PMM.
 *
 * The heap is a singly-linked list of blocks. Each block starts with a small
 * header (size, free flag, pointer to the next block), immediately followed
 * by its usable memory:
 *
 *   [header][.... usable memory ....][header][.... usable memory ....]...
 *
 * kmalloc walks the list looking for the first free block big enough
 * (first-fit). If the block is bigger than needed, it's split: the leftover
 * tail becomes its own free block. kfree just flips the free flag back on,
 * then tries to merge with the block right after it if that one is also
 * free and physically touching — this is what keeps repeated alloc/free
 * cycles from fragmenting the heap into useless slivers.
 *
 * Each block's memory comes from ONE PMM frame (4 KB). Frames from different
 * kmalloc calls need not be physically adjacent — the list just chains them
 * together regardless of address. The one real limit this creates: a single
 * allocation can't be bigger than one frame minus its header. Fine for now;
 * every struct the kernel needs so far is tiny. Revisit if that changes. */

#include <stdint.h>
#include "heap.h"
#include "pmm.h"
#include "terminal.h"

typedef struct block_header {
	size_t               size;   /* usable bytes AFTER this header    */
	int                  free;
	struct block_header* next;
} block_header_t;

#define ALIGN 8

static block_header_t* heap_head = NULL;

static size_t align_up(size_t n) {
	return (n + (ALIGN - 1)) & ~(size_t)(ALIGN - 1);
}

/* Pull one more frame from the PMM, turn all of it into a fresh free block,
 * and chain it onto the end of the list. */
static block_header_t* heap_grow(void) {
	uint32_t frame = pmm_alloc_frame();
	if (!frame)
		return NULL;              /* physical memory exhausted */

	block_header_t* block = (block_header_t*)frame;
	block->size = FRAME_SIZE - sizeof(block_header_t);
	block->free = 1;
	block->next = NULL;

	if (!heap_head) {
		heap_head = block;
	} else {
		block_header_t* tail = heap_head;
		while (tail->next)
			tail = tail->next;
		tail->next = block;
	}
	return block;
}

void kheap_init(void) {
	heap_grow();
	term_setcolor(vga_color(VGA_LIGHT_GREEN, VGA_BLACK));
	term_write("Heap ready.\n");
}

void* kmalloc(size_t size) {
	size = align_up(size);
	if (size == 0)
		return NULL;

	for (block_header_t* b = heap_head; b; b = b->next) {
		if (!b->free || b->size < size)
			continue;

		/* Enough room left over to be worth splitting into its own block? */
		if (b->size >= size + sizeof(block_header_t) + ALIGN) {
			block_header_t* rest =
			    (block_header_t*)((uint8_t*)b + sizeof(block_header_t) + size);
			rest->size = b->size - size - sizeof(block_header_t);
			rest->free = 1;
			rest->next = b->next;
			b->next = rest;
			b->size = size;
		}
		b->free = 0;
		return (uint8_t*)b + sizeof(block_header_t);
	}

	/* Nothing free enough fits — grab a fresh frame and try exactly once
	 * more. (If `size` alone is bigger than a frame, this still fails; see
	 * the file header note.) */
	if (heap_grow())
		return kmalloc(size);

	return NULL;   /* physical memory exhausted */
}

void kfree(void* ptr) {
	if (!ptr)
		return;

	block_header_t* b = (block_header_t*)((uint8_t*)ptr - sizeof(block_header_t));
	b->free = 1;

	/* Merge forward only: if the NEXT block in the list is free and its
	 * address is exactly where this block's data ends, they're physically
	 * touching (same frame, from an earlier split) — fold them into one.
	 * We don't merge backward (would need a previous-pointer or an O(n)
	 * scan from the head); a known simplification, same spirit as the
	 * IRQ-context shortcut in M5. */
	block_header_t* next = b->next;
	if (next && next->free &&
	    (uint8_t*)b + sizeof(block_header_t) + b->size == (uint8_t*)next) {
		b->size += sizeof(block_header_t) + next->size;
		b->next  = next->next;
	}
}
