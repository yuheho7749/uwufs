/**
 * Provide low level block operations
 *
 * Author: Joseph
 */

#ifndef LOW_LEVEL_OPERATIONS_H
#define LOW_LEVEL_OPERATIONS_H

#include "uwufs.h"

#include <stdlib.h>

/**
 * Reads an entire block from the specified block device.
 * The block size is determined by UWUFS_BLOCK_SIZE (see uwufs.h)
 *
 * `fd`: block device
 * `buf`: output var to be read into (size must be at least UWUFS_BLOCK_SIZE)
 * `blk_num`: block number to be read
 */
ssize_t read_blk(int fd, void* buf, uwufs_blk_t blk_num);

/**
 * Writes an entire block to the specified block device.
 * The buf size must be exactly UWUFS_BLOCK_SIZE (see uwufs.h)
 *
 * `fd`: block device
 * `buf`: data to write to block device (size must be UWUFS_BLOCK_SIZE)
 * `blk_num`: block number to write to
 */
ssize_t write_blk(int fd, const void* buf, uwufs_blk_t blk_num);

/**
 * Reads an inode from the specified block device.
 * Caution: it assumes the start of ilist is at constant offset determined
 * 		by UWUFS_RESERVED_SPACE (might change if implementation changes)
 *
 * `fd`: block device
 * `buf`: output var to be read into (must be at least the
 * 		sizeof(struct uwufs_inode))
 * `inode_num`: inode number to be read
 */
ssize_t read_inode(int fd, void* buf, uwufs_blk_t inode_num);

/**
 * Writes an inode to the specified block device.
 * Caution: it assumes the start of ilist is at constant offset determined
 * 		by UWUFS_RESERVED_SPACE (might change if implementation changes)
 *
 * `fd`: block device
 * `buf`: inode data to write to block device
 * `size`: size of buf (should be sizeof(struct uwufs_inode) unless you 
 * 		know exactly what you are doing!)
 * `inode_num`: inode number to write to
 */
ssize_t write_inode(int fd, const void* buf, size_t size, uwufs_blk_t inode_num);

/**
 * Allocates a free block from device.
 *
 * `fd`: block device
 * `blk_num`: output var will contain the blk number of allocated block
 */
ssize_t malloc_blk(int fd, uwufs_blk_t *blk_num);

/**
 * Frees and returns a already allocated block to freelist.
 * Caution: this function does not check if the blk is already free
 * 		nor if it is a data block at all.
 *
 * `fd`: block device
 * `blk_num`: blk number of data block to be freed
 */
ssize_t free_blk(int fd, const uwufs_blk_t blk_num);

/**
 * TODO: Not implemented yet
 * Finds a free inode and returns it's inode number in the `inode_num`
 * 		output variable
 *
 * `fd`: block device
 * `inode_num`: output var will contain the inode number of a free inode
 */
ssize_t find_free_inode(int fd, uwufs_blk_t *inode_num);

/**
 * TODO:
 * Find the inode of the corresponding file. If root directory inode is
 * 		valid, it will use it. Otherwise, it will read the super blk for
 * 		the root directory.
 *
 * `fd`: block device
 * `path`: null terminated file path
 * `root_dir_inode`: can be optionally provided to save a super blk read
 */
ssize_t namei(int fd, const char *path, const struct uwufs_inode *root_dir_inode);

#endif
