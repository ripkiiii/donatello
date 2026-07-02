/* irq.c — remap the PIC, dispatch hardware interrupts to handlers, and
 * acknowledge them (EOI) so more can arrive. */

#include "irq.h"
#include "io.h"

#define PIC1_CMD   0x20   /* master PIC command port */
#define PIC1_DATA  0x21   /* master PIC data port    */
#define PIC2_CMD   0xA0   /* slave PIC command port  */
#define PIC2_DATA  0xA1   /* slave PIC data port     */
#define PIC_EOI    0x20   /* "End Of Interrupt" byte */

static irq_handler_t handlers[16];

void pic_remap(void) {
	/* ICW1: begin initialization (expect ICW2..ICW4). */
	outb(PIC1_CMD, 0x11); io_wait();
	outb(PIC2_CMD, 0x11); io_wait();

	/* ICW2: the new vector offsets — master IRQs -> 32, slave IRQs -> 40. */
	outb(PIC1_DATA, 0x20); io_wait();
	outb(PIC2_DATA, 0x28); io_wait();

	/* ICW3: wire master and slave together (slave hangs off master IRQ2). */
	outb(PIC1_DATA, 0x04); io_wait();
	outb(PIC2_DATA, 0x02); io_wait();

	/* ICW4: use 8086 mode. */
	outb(PIC1_DATA, 0x01); io_wait();
	outb(PIC2_DATA, 0x01); io_wait();

	/* Unmask everything (0 = enabled). */
	outb(PIC1_DATA, 0x00);
	outb(PIC2_DATA, 0x00);
}

void irq_install_handler(int irq, irq_handler_t handler) {
	handlers[irq] = handler;
}

/* Called from irq_common (isr.s) for every hardware interrupt. */
void irq_handler(registers_t* regs) {
	int irq = regs->int_no - 32;

	if (handlers[irq])
		handlers[irq](regs);

	/* Acknowledge the interrupt. If it came from the slave PIC, tell it too,
	 * then always tell the master — otherwise no more IRQs get through. */
	if (irq >= 8)
		outb(PIC2_CMD, PIC_EOI);
	outb(PIC1_CMD, PIC_EOI);
}
