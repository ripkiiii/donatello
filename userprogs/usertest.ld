/* usertest.ld — links usertest.c to run at 3 MiB. Deliberately a different
 * address from hello.ld's 2 MiB (M7) — the two programs are never run at
 * the same time, but keeping them distinct avoids any confusion about
 * which blob is mapped where while reading the code. */
ENTRY(_start)

SECTIONS
{
	. = 3M;

	.text   BLOCK(4K) : ALIGN(4K) { *(.text) }
	.rodata BLOCK(4K) : ALIGN(4K) { *(.rodata) }
	.data   BLOCK(4K) : ALIGN(4K) { *(.data) }
	.bss    BLOCK(4K) : ALIGN(4K) { *(COMMON) *(.bss) }
}
