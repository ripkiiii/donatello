/* ata.h — read sectors off the (virtual) disk QEMU gives us, PIO mode.
 *
 * "PIO" (Programmed I/O) means the CPU itself shuffles every byte through a
 * port, one instruction at a time — no DMA, no interrupts, just polling a
 * status register until the disk says it's ready. It's the oldest, simplest
 * way to talk to an ATA disk, which makes it exactly right for a first
 * driver: slow by modern standards, but there's no hidden machinery. */
#ifndef ATA_H
#define ATA_H

#include <stdint.h>

#define ATA_SECTOR_SIZE 512

/* Reads ONE 512-byte sector at LBA (Logical Block Address) `lba` from the
 * primary ATA bus, master drive, into `buffer` (must have room for at
 * least ATA_SECTOR_SIZE bytes). Returns 1 on success, 0 if the drive
 * reported an error. */
int ata_read_sector(uint32_t lba, uint8_t* buffer);

#endif
