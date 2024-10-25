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

typedef uint64_t uwufs_blk_t; 	// 64 bits (8 bytes)
typedef char uwufs_file_name_t[56];

struct uwufs_super_blk {
	uwufs_blk_t total_blks;
	uwufs_blk_t freelist_head;
	uwufs_blk_t ilist_start;
	uwufs_blk_t ilist_size;
};

struct uwufs_inode {
	uwufs_blk_t direct_blks[10];
	uwufs_blk_t single_indirect_blks;
	uwufs_blk_t double_indirect_blks;
	uwufs_blk_t triple_indirect_blks;
	unsigned int file_metadata; 			// file types
	// Handle permissions etc later
};

struct uwufs_directory_file_entry {
	uwufs_file_name_t file_name;
	uwufs_blk_t inode;
};

struct uwufs_directory_blk {
	struct uwufs_directory_file_entry file_entries[64];
};

struct uwufs_regular_file_blk {
	char data[UWUFS_BLOCK_SIZE];
};

#endif
