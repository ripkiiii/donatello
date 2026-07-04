/* fs.h — the simplest filesystem that could work: a flat table of files on
 * disk, no directories, no fragmentation — every file's sectors are
 * contiguous, described by one entry in a table stored in sector 0.
 *
 * Real filesystems (FAT, ext2, ...) exist to solve problems we don't have
 * yet: files that grow or shrink over time, many small files reusing
 * freed space, directory hierarchies. Ours is built once, offline, by a
 * host-side tool (tools/mkdisk.py) and never modified at runtime — there
 * is no "write" path at all. That's a real, deliberate scope limit: this
 * milestone is about FINDING and LOADING a program by name, not about a
 * mutable filesystem. */
#ifndef FS_H
#define FS_H

#include <stdint.h>

#define FS_NAME_MAX 24

/* One entry, exactly as tools/mkdisk.py writes it and fs_init() reads it
 * back — 24 + 4 + 4 = 32 bytes, so 512/32 = 16 entries fit in one sector
 * with no padding. An entry with name[0] == '\0' marks the end of the
 * table (there's no separate "count" field; that byte IS the sentinel). */
typedef struct {
	char     name[FS_NAME_MAX];
	uint32_t start_lba;
	uint32_t size_bytes;
} __attribute__((packed)) fs_entry_t;

/* Reads the file table (disk sector 0) into memory. Call once at boot,
 * after the ATA driver is usable. */
void fs_init(void);

/* How many files are on disk — for listing (the `ls` shell command). */
int fs_count(void);

/* The i'th file table entry (0 <= i < fs_count()). NULL if out of range. */
const fs_entry_t* fs_entry_at(int i);

/* Looks up `name`, reads its full contents off disk into a fixed internal
 * buffer, and returns a pointer to it — NULL if the file doesn't exist,
 * or is bigger than the buffer can hold (deliberately NOT kmalloc here:
 * M6c's heap caps a single allocation at one 4 KB frame, too small for a
 * whole program; a dedicated static buffer sidesteps that cleanly and
 * honestly, rather than fighting the allocator for a use case it was
 * never sized for). Sets *out_size to the file's length in bytes. */
const uint8_t* fs_load(const char* name, uint32_t* out_size);

#endif
