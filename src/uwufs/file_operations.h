/**
 * File operations for uwufs.
 *
 * Author: Joseph
 */

#ifndef FILE_OPERATIONS_H
#define FILE_OPERATIONS_H

#include "uwufs.h"
#include <stdlib.h>

// NOTE: Not Implemented yet (feel free to change function signature)
// 		(see syscall.c `__create_regular_file` for a similar function)
/**
 * Creates a file using file mode. The resulting file inode is
 * 		saved in `inode` output var
 *
 * `inode`: output var for inode of newly created file
 * `flags`: file type/permissions
 */
ssize_t create_file(uwufs_blk_t *inode, uint16_t mode);

/**
 * Assumes the caller has already allocated a directory data blk.
 * Unless you want to specify/provide which directory data blk to
 * put the entry into, you should use `add_directory_file_entry` instead.
 *
 * Return: 0 on success or -ENOSPC if the directory block is full
 *
 * Parameters:
 * `dir_blk`: a valid data block
 * `name`: name of the new file
 * `file_inode_num`: file inode to associate `name` to
 */
ssize_t put_directory_file_entry(struct uwufs_directory_data_blk *dir_blk,
								 const char name[UWUFS_FILE_NAME_SIZE],
								 uwufs_blk_t file_inode_num);

/**
 * Adds a file entry to the specified directory. This function
 * will automatically allocate additional data blocks if needed.
 *
 * Return: 0 on success or -ENOSPC if the directory is full/no more
 * 		space on storage device (no more blocks can be allocated)
 *
 * TODO: Indirect blks; currently only checks/allocates direct blks
 * 
 * Parameters:
 * `fd`: block device
 * `dir_inode_num`: a valid data block number
 * `name`: name of the new file
 * `file_inode_num`: file inode to associate `name` to
 * `nlinks_change`: change to the file links count
 */
ssize_t add_directory_file_entry(int fd,
								 const uwufs_blk_t dir_inode_num,
								 const char name[UWUFS_FILE_NAME_SIZE],
								 uwufs_blk_t file_inode_num,
								 int nlinks_change);

ssize_t split_path_parent_child(const char *path,
								char *parent_path,
								char *child_dir);

ssize_t __remove_entry_from_dir_data_blk(int fd,
										 uwufs_blk_t dir_data_blk_num,
										 const char name[UWUFS_FILE_NAME_SIZE],
										 uwufs_blk_t file_inode_num);

ssize_t unlink_file(int fd,
					  const char *path,
					  struct uwufs_inode *inode,
					  uwufs_blk_t inode_num);

ssize_t remove_file(int fd,
					  struct uwufs_inode *inode,
					  uwufs_blk_t inode_num);
#endif
