/* gdt.h — set up our own Global Descriptor Table. */
#ifndef GDT_H
#define GDT_H

/* Build a flat GDT (null + kernel code + kernel data) and load it. */
void gdt_init(void);

#endif
