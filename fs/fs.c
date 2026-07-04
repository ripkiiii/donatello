/* fs.c — read the flat file table off disk, and load a named file's
 * contents into a fixed buffer for the ELF loader to parse. */

#include "fs.h"
#include "ata.h"
#include "string.h"
#include "terminal.h"

#define FS_MAX_ENTRIES (ATA_SECTOR_SIZE / sizeof(fs_entry_t))   /* 16 */

static fs_entry_t table[FS_MAX_ENTRIES];
static int        table_count;

/* One file loaded at a time, no concurrency to worry about — plenty of
 * room for anything this milestone's test programs need (a few KB each). */
#define LOAD_BUFFER_SIZE (64 * 1024)
static uint8_t load_buffer[LOAD_BUFFER_SIZE];

void fs_init(void) {
	uint8_t sector[ATA_SECTOR_SIZE];
	ata_read_sector(0, sector);

	const fs_entry_t* entries = (const fs_entry_t*)sector;
	table_count = 0;
	for (int i = 0; i < (int)FS_MAX_ENTRIES; i++) {
		if (entries[i].name[0] == '\0')
			break;                       /* sentinel: end of table */
		table[table_count++] = entries[i];
	}
}

int fs_count(void) {
	return table_count;
}

const fs_entry_t* fs_entry_at(int i) {
	if (i < 0 || i >= table_count)
		return NULL;
	return &table[i];
}

static const fs_entry_t* fs_find(const char* name) {
	for (int i = 0; i < table_count; i++)
		if (strcmp(table[i].name, name) == 0)
			return &table[i];
	return NULL;
}

const uint8_t* fs_load(const char* name, uint32_t* out_size) {
	const fs_entry_t* e = fs_find(name);
	if (!e) {
		term_setcolor(vga_color(VGA_LIGHT_RED, VGA_BLACK));
		term_write("fs: no such file: ");
		term_write(name);
		term_write("\n");
		term_setcolor(vga_color(VGA_WHITE, VGA_BLACK));
		return NULL;
	}

	if (e->size_bytes > LOAD_BUFFER_SIZE) {
		term_setcolor(vga_color(VGA_LIGHT_RED, VGA_BLACK));
		term_write("fs: file too big for the load buffer\n");
		term_setcolor(vga_color(VGA_WHITE, VGA_BLACK));
		return NULL;
	}

	uint32_t sectors = (e->size_bytes + ATA_SECTOR_SIZE - 1) / ATA_SECTOR_SIZE;
	for (uint32_t i = 0; i < sectors; i++) {
		if (!ata_read_sector(e->start_lba + i, load_buffer + i * ATA_SECTOR_SIZE)) {
			term_setcolor(vga_color(VGA_LIGHT_RED, VGA_BLACK));
			term_write("fs: disk read error\n");
			term_setcolor(vga_color(VGA_WHITE, VGA_BLACK));
			return NULL;
		}
	}

	if (out_size)
		*out_size = e->size_bytes;
	return load_buffer;
}
