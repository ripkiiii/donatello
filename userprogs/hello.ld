/* hello.ld — links hello.c to run at 2 MiB, comfortably clear of the kernel
 * (which sits at 1 MiB and ends well under 1.1 MiB) and clear of the frames
 * PMM/paging/heap have claimed so far. Since paging identity-maps ALL RAM,
 * any unclaimed address like this one is backed by real, usable memory —
 * no new page-table entries needed for this milestone. */
ENTRY(_start)

SECTIONS
{
	. = 2M;

	.text   BLOCK(4K) : ALIGN(4K) { *(.text) }
	.rodata BLOCK(4K) : ALIGN(4K) { *(.rodata) }
	.data   BLOCK(4K) : ALIGN(4K) { *(.data) }
	.bss    BLOCK(4K) : ALIGN(4K) { *(COMMON) *(.bss) }
}
