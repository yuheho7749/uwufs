/**
 * 	Provide low level block operations
 *
 * 	Author: Joseph
 */

#ifndef LOW_LEVEL_OPERATIONS_H
#define LOW_LEVEL_OPERATIONS_H

#include "uwufs.h"

#include <stdlib.h>

/**
 * 	Reads an entire block from the specified block device.
 * 	The block size is determined by UWUFS_BLOCK_SIZE (see uwufs.h)
 *
 * 	`fd`: block device
 * 	`buf`: output var to be read into (must be at least UWUFS_BLOCK_SIZE)
 * 	`blk_num`: block number to be read
 */
ssize_t read_blk(int fd, void* buf, uwufs_blk_t blk_num);

/**
 * 	Writes an entire block to the specified block device.
 * 	The buf size must be exactly UWUFS_BLOCK_SIZE (see uwufs.h)
 *
 * 	`fd`: block device
 * 	`buf`: data to write to block device (must be exactly UWUFS_BLOCK_SIZE)
 * 	`size`: must be UWUFS_BLOCK_SIZE (might change if implementation changes)
 * 	`blk_num`: block number to write to
 */
ssize_t write_blk(int fd, const void* buf, size_t size, uwufs_blk_t blk_num);

/**
 * 	Reads an inode from the specified block device.
 * 	Caution: it assumes the start of ilist is at constant offset determined
 * 		by UWUFS_RESERVED_SPACE (might change if implementation changes)
 *
 * 	`fd`: block device
 * 	`buf`: output var to be read into (must be at least the
 * 		sizeof(struct uwufs_inode))
 * 	`inode_num`: inode number to be read
 */
ssize_t read_inode(int fd, void* buf, uwufs_blk_t inode_num);

/**
 * 	Writes an inode to the specified block device.
 * 	Caution: it assumes the start of ilist is at constant offset determined
 * 		by UWUFS_RESERVED_SPACE (might change if implementation changes)
 *
 * 	`fd`: block device
 * 	`buf`: inode data to write to block device
 * 	`size`: size of buf (should be sizeof(struct uwufs_inode) unless you 
 * 		know exactly what you are doing!)
 * 	`inode_num`: inode number to write to
 */
ssize_t write_inode(int fd, const void* buf, size_t size, uwufs_blk_t inode_num);


#endif
