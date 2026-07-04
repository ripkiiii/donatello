/* ata.c — ATA PIO mode, primary bus, master drive, LBA28 addressing.
 *
 * The primary ATA controller lives at a fixed set of I/O ports (this is a
 * PC legacy convention going back to the original IBM PC/AT, which QEMU
 * faithfully emulates regardless of how the kernel itself was booted):
 *
 *   0x1F0  data register        — read/write sector bytes here, 16 bits
 *                                 at a time (256 words = 512 bytes)
 *   0x1F2  sector count         — how many sectors this command covers
 *   0x1F3  LBA low   (bits 0-7)
 *   0x1F4  LBA mid   (bits 8-15)
 *   0x1F5  LBA high  (bits 16-23)
 *   0x1F6  drive/head           — bit 6 selects LBA mode (vs old CHS),
 *                                 bits 0-3 are LBA bits 24-27, bit 4
 *                                 selects master (0) vs slave (1) drive
 *   0x1F7  command/status       — write a command, read status back
 *
 * This is LBA28: 28 address bits total (4 in the drive/head register, 24
 * across the three LBA registers) — enough to address 128 GiB, plenty for
 * a hobby OS's disk image. */

#include "ata.h"
#include "io.h"

#define ATA_DATA        0x1F0
#define ATA_SECTOR_CNT  0x1F2
#define ATA_LBA_LOW     0x1F3
#define ATA_LBA_MID     0x1F4
#define ATA_LBA_HIGH    0x1F5
#define ATA_DRIVE_HEAD  0x1F6
#define ATA_COMMAND     0x1F7
#define ATA_STATUS      0x1F7

#define CMD_READ_SECTORS 0x20

#define STATUS_ERR 0x01   /* bit 0: an error occurred                */
#define STATUS_DRQ 0x08   /* bit 3: drive has data ready to transfer */
#define STATUS_BSY 0x80   /* bit 7: drive is busy — wait it out      */

/* Block until the drive isn't busy. Nothing here has a timeout: on real
 * failing hardware this could spin forever, but for our fixed, always-
 * present QEMU virtual disk that's not a scenario we need to handle yet —
 * an honest simplification, same spirit as earlier milestones. */
static void ata_wait_ready(void) {
	while (inb(ATA_STATUS) & STATUS_BSY)
		;
}

int ata_read_sector(uint32_t lba, uint8_t* buffer) {
	ata_wait_ready();

	/* 0xE0 = LBA mode (bit 6) + master drive (bit 4) + bits 24-27 of the
	 * address in the low nibble. */
	outb(ATA_DRIVE_HEAD, 0xE0 | ((lba >> 24) & 0x0F));
	outb(ATA_SECTOR_CNT, 1);                 /* one sector this command */
	outb(ATA_LBA_LOW,  (uint8_t)(lba));
	outb(ATA_LBA_MID,  (uint8_t)(lba >> 8));
	outb(ATA_LBA_HIGH, (uint8_t)(lba >> 16));
	outb(ATA_COMMAND, CMD_READ_SECTORS);

	/* Poll until the drive is done thinking (BSY clears) and either has
	 * data ready (DRQ set) or is reporting an error. */
	uint8_t status;
	do {
		status = inb(ATA_STATUS);
	} while ((status & STATUS_BSY) && !(status & STATUS_ERR));

	if (status & STATUS_ERR)
		return 0;

	/* Sector = 256 sixteen-bit words. The data register hands them out in
	 * order; nothing to do but read exactly that many. */
	uint16_t* dest = (uint16_t*)buffer;
	for (int i = 0; i < ATA_SECTOR_SIZE / 2; i++)
		dest[i] = inw(ATA_DATA);

	return 1;
}
