/* irq.h — hardware interrupts (IRQs): remap the PIC and route each IRQ to a
 * registered handler. */
#ifndef IRQ_H
#define IRQ_H

#include "idt.h"

typedef void (*irq_handler_t)(registers_t*);

/* Move the PIC's interrupts out of the 0..15 range (which collides with CPU
 * exceptions) to 32..47. */
void pic_remap(void);

/* Register a C handler for a given IRQ line (0..15). */
void irq_install_handler(int irq, irq_handler_t handler);

#endif
