/* gdt.c — build a flat Global Descriptor Table and install it.
 *
 * "Flat" means every segment covers the entire 4 GiB address space, so
 * segmentation effectively gets out of the way and we work with plain
 * linear addresses. Later, paging does the real memory management.
 */

#include <stdint.h>
#include "gdt.h"

/* One GDT entry (descriptor) — the CPU demands exactly this 8-byte layout,
 * with the base and limit fields awkwardly split across it. We use
 * __attribute__((packed)) so the compiler doesn't insert padding. */
struct gdt_entry {
	uint16_t limit_low;    /* limit bits 0..15                        */
	uint16_t base_low;     /* base  bits 0..15                        */
	uint8_t  base_mid;     /* base  bits 16..23                       */
	uint8_t  access;       /* present, ring, code/data, exec, r/w     */
	uint8_t  flags_limit;  /* limit bits 16..19 + granularity flags   */
	uint8_t  base_high;    /* base  bits 24..31                       */
} __attribute__((packed));

/* What the `lgdt` instruction wants: table size (minus 1) + its address. */
struct gdt_ptr {
	uint16_t limit;
	uint32_t base;
} __attribute__((packed));

static struct gdt_entry gdt[3];
static struct gdt_ptr   gdt_pointer;

/* Defined in gdt_flush.s — loads the GDT and reloads the segment registers. */
extern void gdt_flush(uint32_t gdt_ptr_addr);

/* Fill descriptor `i` from a human-friendly base / limit / flag set,
 * scattering the bits into the weird hardware layout above. */
static void gdt_set_entry(int i, uint32_t base, uint32_t limit,
                          uint8_t access, uint8_t gran) {
	gdt[i].base_low    =  base        & 0xFFFF;
	gdt[i].base_mid    = (base >> 16) & 0xFF;
	gdt[i].base_high   = (base >> 24) & 0xFF;

	gdt[i].limit_low   =  limit        & 0xFFFF;
	gdt[i].flags_limit = ((limit >> 16) & 0x0F) | (gran & 0xF0);

	gdt[i].access      = access;
}

void gdt_init(void) {
	gdt_pointer.limit = sizeof(gdt) - 1;
	gdt_pointer.base  = (uint32_t)&gdt;

	/*                 base  limit       access  gran                     */
	gdt_set_entry(0,   0,    0,          0x00,   0x00); /* null (required) */
	gdt_set_entry(1,   0,    0xFFFFFFFF, 0x9A,   0xCF); /* kernel code     */
	gdt_set_entry(2,   0,    0xFFFFFFFF, 0x92,   0xCF); /* kernel data     */

	gdt_flush((uint32_t)&gdt_pointer);
}
