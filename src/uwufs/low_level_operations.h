/**
 * 	Provide low level block operations
 *
 * 	Author: Joseph
 */

#ifndef LOW_LEVEL_OPERATIONS_H
#define LOW_LEVEL_OPERATIONS_H

#include "uwufs.h"

#include <stdlib.h>

ssize_t read_blk(int fd, void* buf, uwufs_blk_t blk_num);
ssize_t write_blk(int fd, const void* buf, size_t size, uwufs_blk_t blk_num);

ssize_t read_inode(int fd, void* buf, uwufs_blk_t inode_num);
ssize_t write_inode(int fd, const void* buf, size_t size, uwufs_blk_t inode_num);


#endif
