/* gdt.c — build a flat Global Descriptor Table and install it.
 *
 * "Flat" means every segment covers the entire 4 GiB address space, so
 * segmentation effectively gets out of the way and we work with plain
 * linear addresses. Paging does the real memory management.
 *
 * M8 adds three entries: a USER code segment, a USER data segment (both
 * DPL=3 — the only thing that actually separates "user" from "kernel" at
 * the segment level), and a TSS (Task State Segment). We don't do real
 * hardware task-switching — the TSS exists purely so the CPU knows which
 * kernel stack to switch to whenever an interrupt fires while we're
 * running in ring 3. Without it, a ring 3 -> ring 0 transition would try
 * to use the CPU's CURRENT (user) stack for kernel state, which is exactly
 * the kind of privilege violation rings exist to prevent. */

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

/* The i386 TSS. Real hardware task-switching would use nearly every field
 * here; we only ever load esp0/ss0 (the rest stays zeroed) because our
 * only use for it is "which stack does the CPU switch to on a privilege
 * change" — not actually switching between multiple independent tasks. */
struct tss_entry {
	uint32_t prev_tss;
	uint32_t esp0;    /* stack pointer to load when entering ring 0 */
	uint32_t ss0;     /* stack segment  to load when entering ring 0 */
	uint32_t esp1, ss1, esp2, ss2;
	uint32_t cr3, eip, eflags;
	uint32_t eax, ecx, edx, ebx, esp, ebp, esi, edi;
	uint32_t es, cs, ss, ds, fs, gs;
	uint32_t ldt;
	uint16_t trap;
	uint16_t iomap_base;
} __attribute__((packed));

#define GDT_ENTRIES 6   /* null, kernel CS, kernel DS, user CS, user DS, TSS */
#define TSS_SELECTOR 0x28

static struct gdt_entry gdt[GDT_ENTRIES];
static struct gdt_ptr   gdt_pointer;
static struct tss_entry tss;

/* Defined in gdt_flush.s — loads the GDT, reloads segment registers, and
 * loads the task register (`ltr`) so the CPU knows where the TSS is. */
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
	/* Same access bytes as kernel code/data, but DPL bits (6:5) set to 11
	 * instead of 00 — 0x9A -> 0xFA, 0x92 -> 0xF2. This one bit-field change
	 * is the entire difference between "kernel segment" and "user segment"
	 * at the GDT level; everything else about isolation comes from paging
	 * (the U/S bit per page) and the CPU's own ring-check on instructions. */
	gdt_set_entry(3,   0,    0xFFFFFFFF, 0xFA,   0xCF); /* user code       */
	gdt_set_entry(4,   0,    0xFFFFFFFF, 0xF2,   0xCF); /* user data       */

	/* TSS descriptor: base/limit point at our tss struct (not a 4 GB flat
	 * region like the others — the CPU only ever reads specific offsets
	 * into it, so its "segment" is just its own small size). Access 0x89 =
	 * present, DPL 0, type 0x9 (32-bit TSS, available/not-busy). */
	for (int i = 0; i < (int)sizeof(tss); i++)
		((uint8_t*)&tss)[i] = 0;
	tss.ss0 = KERNEL_DS;
	gdt_set_entry(5, (uint32_t)&tss, sizeof(tss) - 1, 0x89, 0x00);

	gdt_flush((uint32_t)&gdt_pointer);
}

void tss_set_kernel_stack(uint32_t esp0) {
	tss.esp0 = esp0;
}
