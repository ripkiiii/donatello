# idt_load.s — point the CPU's IDTR register at our table.

.global idt_load
.type idt_load, @function
idt_load:
	mov 4(%esp), %eax   # first argument: address of our idt_ptr struct
	lidt (%eax)         # tell the CPU where the IDT lives
	ret
