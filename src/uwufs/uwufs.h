/**
 * Define macros and common objects for uwufs.
 *
 * Authors: Joseph, Kay
 * Collaborators: Kong, Jason
 */

#ifndef UWUFS_H
#define UWUFS_H

#include <stdint.h>

// Must be in 4k bytes block (4096 bytes)
#define UWUFS_BLOCK_SIZE 				4096

/* uwufs defaults (can be changed) */
#define UWUFS_ILIST_DEFAULT_PERCENTAGE 	0.1f
#define UWUFS_INODE_DEFAULT_SIZE		256

#define UWUFS_DIRECT_BLOCKS				10
#define UWUFS_INDIRECT_BLOCKS			1
#define UWUFS_DOUBLE_INDIRECT_BLOCKS	1
#define UWUFS_TRIPLE_INDIRECT_BLOCKS	1

#define UWUFS_ROOT_DIR_INODE 			2
#define UWUFS_RESERVED_SPACE			1

// Total file entry in a directory is 256 bytes
#define UWUFS_FILE_NAME_SIZE			248 // includes null-terminator

/* File access mode */
typedef uint16_t uwufs_aflags_t; // Deprecated (use uint16_t directly)
// File types
#define F_TYPE_BITS						0b1111000000000000
// NOTE: Might change to match stat.h file types?
#define F_TYPE_FREE						0
#define F_TYPE_REGULAR					(1 << 12)
#define F_TYPE_DIRECTORY				(2 << 12)
#define F_TYPE_SYMLINK 					(3 << 12)
// File permissions (12 bits: SUID, SGID, Sticky, rwxrwxrwx)
#define F_PERM_BITS						0b0000111111111111
#define F_PERM_SUID						(1 << 11) 	// 04000 (octal)
#define F_PERM_SGID						(1 << 10) 	// 02000 (octal)
#define F_PERM_STICKY					(1 << 9) 	// 01000 (octal)


typedef uint64_t uwufs_blk_t; 	// 64 bits (8 bytes)
typedef char uwufs_file_name_t[UWUFS_FILE_NAME_SIZE];

// PLAN: Metadata blk will be 2nd block (useful for fsck/sanity checks)
// 		Should handle the following without a blind memcpy because of
// 		potentially different endianness/architecture for removable media.
// 		Also should be separate from super blk because this info should
// 		be written once at mkfs and never changed again.
// 	- uwufs version
// 	- endianness
// 	- architecture (x86_64)

struct __attribute__((__packed__)) uwufs_super_blk {
	uwufs_blk_t total_blks;
	uwufs_blk_t ilist_start;
	uwufs_blk_t ilist_total_size;
	uwufs_blk_t freelist_start;
	uwufs_blk_t freelist_total_size;
	uwufs_blk_t freelist_head;

	char padding[UWUFS_BLOCK_SIZE - (6 * sizeof(uwufs_blk_t))];
};

// 256 bytes for larger {a,m,c}times etc
struct __attribute__((__packed__)) uwufs_inode {
	uwufs_blk_t direct_blks[UWUFS_DIRECT_BLOCKS];
	uwufs_blk_t single_indirect_blks;
	uwufs_blk_t double_indirect_blks;
	uwufs_blk_t triple_indirect_blks;
	uint16_t file_mode; 		// file types/permissions
	uint64_t file_size;
	uint16_t file_links_count;
	uint32_t file_uid;
	uint32_t file_gid;
	uint64_t file_atime;
	uint64_t file_mtime;
	uint64_t file_ctime;

	// NOTE: might want to also track nano seconds for {a,m,c}time
	char padding[128 - 20];
};

struct __attribute__((__packed__)) uwufs_inode_blk {
	struct uwufs_inode inodes[UWUFS_BLOCK_SIZE/sizeof(struct uwufs_inode)];
};

struct __attribute__((__packed__)) uwufs_directory_file_entry {
	uwufs_file_name_t file_name;
	uwufs_blk_t inode_num;
};

struct __attribute__((__packed__)) uwufs_directory_data_blk {
	// 16 entries (assuming 4096 byte block size and 256 bytes per entry)
	struct uwufs_directory_file_entry file_entries[
		UWUFS_BLOCK_SIZE/sizeof(struct uwufs_directory_file_entry)];
};

struct __attribute__((__packed__)) uwufs_indirect_blk {
	uwufs_blk_t entries[UWUFS_BLOCK_SIZE / sizeof(uwufs_blk_t)];
};

struct __attribute__((__packed__)) uwufs_regular_file_data_blk {
	char data[UWUFS_BLOCK_SIZE];
};

struct __attribute__((__packed__)) uwufs_free_data_blk {
	uwufs_blk_t next_free_blk;

	char padding[UWUFS_BLOCK_SIZE - sizeof(uwufs_blk_t)];
};

#define RETURN_IF_ERROR(status) \
    if (status < 0)              \
        return status;

#endif
