/**
 * 	File operations for uwufs.
 *
 * 	Author: Joseph
 */

#ifndef FILE_OPERATIONS_H
#define FILE_OPERATIONS_H

#include "uwufs.h"
#include <stdlib.h>

// TODO:
ssize_t create_file(uwufs_blk_t *inode, uwufs_aflags_t access_flags);

ssize_t create_directory_file_entry(struct uwufs_directory_data_blk *dir_blk,
								 const char name[UWUFS_FILE_NAME_SIZE],
								 uwufs_blk_t file_inode_num);

#endif
