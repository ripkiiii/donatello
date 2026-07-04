# usermode.s — the fake-iret trick that drops us from ring 0 to ring 3.
#
# `iret` normally pops, in order: EIP, CS, EFLAGS, and (only if the CS it
# just popped has a different privilege level than we're currently at) ESP
# and SS too. It does this UNCONDITIONALLY based on what's on the stack —
# it has no idea whether we're resuming a real interrupt or not. So we push
# exactly what a real ring3-bound interrupt frame would look like, then
# `iret`, and the CPU carries us into ring 3 believing nothing unusual
# happened.

.global enter_usermode
.type enter_usermode, @function
enter_usermode:
	# args (cdecl, from the C caller): 4(%esp)=entry point, 8(%esp)=user esp
	mov 4(%esp), %eax
	mov 8(%esp), %ebx

	# Segment registers DS/ES/FS/GS must already be USER_DS (0x23 | RPL 3)
	# before the iret — unlike CS/SS, iret doesn't touch these, so if we
	# left them at the kernel selector, the first instruction after
	# landing in ring 3 that touches memory through ds/es would fault
	# (CPL 3 code may not use a DPL 0 data selector).
	mov $0x23, %cx
	mov %cx, %ds
	mov %cx, %es
	mov %cx, %fs
	mov %cx, %gs

	# Build the frame iret expects, bottom of stack (popped last) to top
	# (popped first): SS, ESP, EFLAGS, CS, EIP.
	push $0x23        # SS   = user data selector (RPL 3)
	push %ebx         # ESP  = user stack top

	pushf
	pop %edx
	or  $0x200, %edx  # force IF (interrupts enabled) — so keyboard/timer
	                  # IRQs still fire while ring 3 code is running
	push %edx         # EFLAGS

	push $0x1B        # CS   = user code selector (RPL 3)
	push %eax         # EIP  = entry point

	iret              # CPL drops to 3 here; never returns to our caller
