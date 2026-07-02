# gdt_flush.s — actually load the GDT into the CPU.
#
# Filling a table in memory does nothing on its own; the CPU only cares once
# we (1) point its GDTR register at the table with `lgdt`, and (2) reload the
# segment registers so they use our new descriptors.

.global gdt_flush
.type gdt_flush, @function
gdt_flush:
	mov 4(%esp), %eax   # first argument: address of our gdt_ptr struct
	lgdt (%eax)         # tell the CPU where the new GDT lives

	# Reload the data segment registers with selector 0x10 (our data
	# descriptor = entry 2, i.e. 2 << 3 = 0x10).
	mov $0x10, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
	mov %ax, %ss

	# CS (the code segment) can't be set with a plain mov — the only way to
	# reload it is a far jump. 0x08 = our code descriptor (entry 1 = 1 << 3).
	ljmp $0x08, $.reload_cs
.reload_cs:
	ret
