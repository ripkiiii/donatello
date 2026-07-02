/* idt.c — build the Interrupt Descriptor Table, install handlers for the 32
 * CPU exceptions, and provide the C-level handler the stubs call into. */

#include <stdint.h>
#include "idt.h"
#include "terminal.h"

/* One IDT entry (a "gate"): where the handler is + how to enter it. */
struct idt_entry {
	uint16_t base_low;   /* handler address bits 0..15                   */
	uint16_t selector;   /* code segment selector in the GDT (0x08)      */
	uint8_t  zero;       /* always zero                                  */
	uint8_t  flags;      /* present + ring + gate type (0x8E)            */
	uint16_t base_high;  /* handler address bits 16..31                  */
} __attribute__((packed));

/* What `lidt` wants: table size (minus 1) + address. */
struct idt_ptr {
	uint16_t limit;
	uint32_t base;
} __attribute__((packed));

static struct idt_entry idt[256];
static struct idt_ptr   idt_pointer;

extern void idt_load(uint32_t idt_ptr_addr);   /* in idt_load.s */

/* The 32 exception stubs, defined in isr.s. */
extern void isr0(void),  isr1(void),  isr2(void),  isr3(void);
extern void isr4(void),  isr5(void),  isr6(void),  isr7(void);
extern void isr8(void),  isr9(void),  isr10(void), isr11(void);
extern void isr12(void), isr13(void), isr14(void), isr15(void);
extern void isr16(void), isr17(void), isr18(void), isr19(void);
extern void isr20(void), isr21(void), isr22(void), isr23(void);
extern void isr24(void), isr25(void), isr26(void), isr27(void);
extern void isr28(void), isr29(void), isr30(void), isr31(void);

/* The 16 hardware-interrupt stubs (vectors 32..47), also in isr.s. */
extern void irq0(void),  irq1(void),  irq2(void),  irq3(void);
extern void irq4(void),  irq5(void),  irq6(void),  irq7(void);
extern void irq8(void),  irq9(void),  irq10(void), irq11(void);
extern void irq12(void), irq13(void), irq14(void), irq15(void);

static void idt_set_entry(int n, uint32_t handler, uint16_t selector, uint8_t flags) {
	idt[n].base_low  =  handler        & 0xFFFF;
	idt[n].base_high = (handler >> 16) & 0xFFFF;
	idt[n].selector  = selector;
	idt[n].zero      = 0;
	idt[n].flags     = flags;
}

void idt_init(void) {
	idt_pointer.limit = sizeof(idt) - 1;
	idt_pointer.base  = (uint32_t)&idt;

	/* Start with every gate empty. */
	for (int i = 0; i < 256; i++)
		idt_set_entry(i, 0, 0, 0);

	/* 0x08 = kernel code selector (GDT entry 1). 0x8E = present, ring 0,
	 * 32-bit interrupt gate. */
	void (*stubs[32])(void) = {
		isr0,  isr1,  isr2,  isr3,  isr4,  isr5,  isr6,  isr7,
		isr8,  isr9,  isr10, isr11, isr12, isr13, isr14, isr15,
		isr16, isr17, isr18, isr19, isr20, isr21, isr22, isr23,
		isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31,
	};
	for (int i = 0; i < 32; i++)
		idt_set_entry(i, (uint32_t)stubs[i], 0x08, 0x8E);

	/* Hardware interrupts at vectors 32..47 (after the PIC remap). */
	void (*irqs[16])(void) = {
		irq0,  irq1,  irq2,  irq3,  irq4,  irq5,  irq6,  irq7,
		irq8,  irq9,  irq10, irq11, irq12, irq13, irq14, irq15,
	};
	for (int i = 0; i < 16; i++)
		idt_set_entry(32 + i, (uint32_t)irqs[i], 0x08, 0x8E);

	idt_load((uint32_t)&idt_pointer);
}

/* Every interrupt funnels here through the common stub in isr.s. */
void isr_handler(registers_t* regs) {
	term_setcolor(vga_color(VGA_LIGHT_RED, VGA_BLACK));
	term_write("  [interrupt] caught vector ");
	term_write_dec(regs->int_no);
	term_write("\n");
}
