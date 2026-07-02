/* idt.h — the Interrupt Descriptor Table: which handler runs for each of the
 * 256 possible interrupts. */
#ifndef IDT_H
#define IDT_H

#include <stdint.h>

/* A snapshot of the CPU built on the stack by our ISR stubs (isr.s) and
 * handed to the C handler. The field order MUST match the push order in the
 * common stub, lowest stack address first. */
typedef struct {
	uint32_t ds;                                      /* pushed by us      */
	uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;  /* pushed by pusha   */
	uint32_t int_no, err_code;                        /* pushed by our stub*/
	uint32_t eip, cs, eflags, useresp, ss;            /* pushed by the CPU */
} registers_t;

void idt_init(void);

#endif
