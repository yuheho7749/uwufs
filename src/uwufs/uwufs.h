/**
 * 	Defines macros and common objects for uwufs.
 *
 *	Author: Joseph
 *	Collaborators: Kay, Kong, Jason
 */

#ifndef UWUFS_H
#define UWUFS_H

#include <stdint.h>

// Must be in 4k bytes block (4096 bytes)
#define UWUFS_BLOCK_SIZE 				4096

/* uwufs defaults (can be changed) */
#define UWUFS_ILIST_DEFAULT_SIZE 		0.1f
#define UWUFS_INODE_DEFAULT_SIZE		128

#define UWUFS_DIRECT_BLOCKS				10
#define UWUFS_INDIRECT_BLOCKS			1
#define UWUFS_DOUBLE_INDIRECT_BLOCKS	1
#define UWUFS_TRIPLE_INDIRECT_BLOCKS	1

#define UWUFS_ROOT_DIR_BLK_NUM			2
#define UWUFS_RESERVED_SPACE			1 	// Maybe for journal

// Total file entry in a directory is 64 bytes
#define UWUFS_FILE_NAME_SIZE			56

/* File access flags */
// File types
#define F_TYPE_BITS						0b1111000000000000
#define F_TYPE_REGULAR					1 << 12
#define F_TYPE_DIRECTORY				2 << 12
#define F_TYPE_SYMLINK 					3 << 12
// File permissions (12 bits: SUID, SGID, Sticky, rwxrwxrwx)
#define F_PERM_BITS						0b0000111111111111
#define F_PERM_SUID						1 << 11
#define F_PERM_SGID						1 << 10
#define F_PERM_STICKY					1 << 9


typedef uint64_t uwufs_blk_t; 	// 64 bits (8 bytes)
typedef char uwufs_file_name_t[UWUFS_FILE_NAME_SIZE];

struct __attribute__((__packed__)) uwufs_super_blk {
	uwufs_blk_t total_blks;
	uwufs_blk_t freelist_head;
	uwufs_blk_t ilist_start;
	uwufs_blk_t ilist_size;
	// Need padding (or memcpy mindfully)
	char padding[UWUFS_BLOCK_SIZE - (4 * sizeof(uwufs_blk_t))];
};

struct __attribute__((__packed__)) uwufs_inode {
	uwufs_blk_t direct_blks[UWUFS_DIRECT_BLOCKS];
	uwufs_blk_t single_indirect_blks;
	uwufs_blk_t double_indirect_blks;
	uwufs_blk_t triple_indirect_blks;
	uint16_t access_flags; 				// file types/permissions
	// TODO: owner, num of links, etc
	
	// Padding to get to 128 bytes
	uint64_t padding1;
	uint64_t padding2;
	uint32_t padding3;
	uint16_t padding4;
};

struct __attribute__((__packed__)) uwufs_inode_blk {
	struct uwufs_inode inodes[UWUFS_BLOCK_SIZE/128];
};

struct __attribute__((__packed__)) uwufs_directory_file_entry {
	uwufs_file_name_t file_name;
	uwufs_blk_t inode_num;
};

struct __attribute__((__packed__)) uwufs_directory_data_blk {
	// 64 entries (assuming 4096 byte block size and 64 bytes per entry)
	struct uwufs_directory_file_entry file_entries[
		UWUFS_BLOCK_SIZE/sizeof(struct uwufs_directory_file_entry)];
};

struct __attribute__((__packed__)) uwufs_regular_file_data_blk {
	char data[UWUFS_BLOCK_SIZE];
};

struct __attribute__((__packed__)) uwufs_free_data_blk {
	uwufs_blk_t next_free_blk;
	char padding[UWUFS_BLOCK_SIZE - sizeof(uwufs_blk_t)];
};

#endif