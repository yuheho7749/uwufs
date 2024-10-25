#ifndef UWUFS_H
#define UWUFS_H

// Must be in 4k bytes block (4096 bytes)
#define UWUFS_BLOCK_SIZE 				4096

/* uwufs defaults (can be changed) */
#define UWUFS_ILIST_DEFAULT_SIZE 		0.1
#define UWUFS_INODE_DEFAULT_SIZE		128

#define UWUFS_DIRECT_BLOCKS				10
#define UWUFS_INDIRECT_BLOCKS			1
#define UWUFS_DOUBLE_INDIRECT_BLOCKS	1
#define UWUFS_TRIPLE_INDIRECT_BLOCKS	1

#define UWUFS_ROOT_DIR_BLK_NUM			2
#define UWUFS_RESERVED_SPACE			1 // Maybe for journal

// Total file entry in a directory is 64 bytes
#define UWUFS_FILE_NAME_SIZE			56
typedef unsigned long long uwufs_blknum_t; // 64 bits (8 bytes)


#endif
