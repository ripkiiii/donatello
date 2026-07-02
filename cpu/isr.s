# isr.s — the low-level entry points for the 32 CPU exceptions.
#
# When an exception fires, the CPU jumps to one of these stubs. Each stub
# pushes a uniform frame (a dummy error code if the CPU didn't push one, plus
# the interrupt number) and jumps to a shared routine that saves the full CPU
# state and calls the C handler.

# Stub for exceptions that do NOT push an error code: push a dummy 0 so every
# frame looks the same.
.macro ISR_NOERR n
	.global isr\n
	.type isr\n, @function
	isr\n:
		push $0
		push $\n
		jmp isr_common
.endm

# Stub for exceptions where the CPU already pushed an error code.
.macro ISR_ERR n
	.global isr\n
	.type isr\n, @function
	isr\n:
		push $\n
		jmp isr_common
.endm

ISR_NOERR 0    # divide by zero
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3    # breakpoint
ISR_NOERR 4    # overflow
ISR_NOERR 5
ISR_NOERR 6    # invalid opcode
ISR_NOERR 7
ISR_ERR   8    # double fault (has error code)
ISR_NOERR 9
ISR_ERR   10
ISR_ERR   11
ISR_ERR   12
ISR_ERR   13   # general protection fault
ISR_ERR   14   # page fault
ISR_NOERR 15
ISR_NOERR 16
ISR_NOERR 17
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_NOERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_NOERR 30
ISR_NOERR 31

.extern isr_handler

# Shared tail: save everything, call C, restore, return.
isr_common:
	pusha                # push edi,esi,ebp,esp,ebx,edx,ecx,eax

	xor %eax, %eax
	mov %ds, %ax
	push %eax            # save the data segment selector

	mov $0x10, %ax       # switch to kernel data segment
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

	push %esp            # pass a pointer to the registers_t we just built
	call isr_handler
	add $4, %esp         # drop that argument

	pop %eax             # restore the original data segment
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

	popa                 # restore edi..eax
	add $8, %esp         # drop int_no + err_code
	iret                 # pop eip,cs,eflags (and ss,esp) — resume

# --- hardware interrupts (IRQs) --------------------------------------------
# After the PIC remap, IRQ n arrives as vector 32+n. Same idea as the ISR
# stubs: push a dummy error code + the vector, then a shared tail.
.macro IRQ num, vec
	.global irq\num
	.type irq\num, @function
	irq\num:
		push $0
		push $\vec
		jmp irq_common
.endm

IRQ 0,  32
IRQ 1,  33
IRQ 2,  34
IRQ 3,  35
IRQ 4,  36
IRQ 5,  37
IRQ 6,  38
IRQ 7,  39
IRQ 8,  40
IRQ 9,  41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47

.extern irq_handler

irq_common:
	pusha

	xor %eax, %eax
	mov %ds, %ax
	push %eax

	mov $0x10, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

	push %esp
	call irq_handler     # different C entry: it also sends the EOI
	add $4, %esp

	pop %eax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

	popa
	add $8, %esp
	iret
