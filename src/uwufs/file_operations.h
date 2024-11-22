/**
 * File operations for uwufs.
 *
 * Authors: Joseph, Kay
 */

#ifndef FILE_OPERATIONS_H
#define FILE_OPERATIONS_H

#include "uwufs.h"
#include <stdlib.h>
#include <stdbool.h>

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

/** 
 * NOTE: Deprecated (or needs to be updated for indirect blocks)
 * Counts and stores the # of entries in the directory
 * in `nentries`
 * 
 * Parameters:
 * `fd`: block device
 * `dir_inode`: the directory inode
 * `nentries`: int to store count
*/
ssize_t count_directory_file_entries(int fd,
								 	 struct uwufs_inode *dir_inode,
								 	 int *nentries);

bool is_directory_empty(int fd, struct uwufs_inode *dir_inode);

ssize_t split_path_parent_child(const char *path,
								char *parent_path,
								char *child_dir);

ssize_t __remove_entry_from_dir_data_blk(int fd,
						 struct uwufs_directory_data_blk *dir_data_blk,
						 struct uwufs_directory_data_blk *last_dir_data_blk,
						 int last_file_entry_index,
						 const char name[UWUFS_FILE_NAME_SIZE],
						 uwufs_blk_t file_inode_num);
/**
 * Removes a file entry in its parent directory
 * 
 *  Parameters:
 * `fd`: block device
 * `path`: path to child entry
 * `inode`: child inode
 * `inode_num`: child inode num
 * `nlinks_change`: change to the parent dir file links count
 * 
 * NOTE: nlinks_change should be 0 unless removing a subdir
 * and then it should be -1
 */
ssize_t unlink_file(int fd,
					  const char *path,
					  struct uwufs_inode *inode,
					  uwufs_blk_t inode_num,
					  int nlinks_change);

/**
 * Frees all blk entries in an indirect block.
 * Does not include the indirect block itself.
 */
ssize_t __free_indirect_blk(int fd,
							struct uwufs_indirect_blk *indirect_blk);

/**
 * Frees all blk entries in a double indirect block recursively,
 * including all the indirect blocks and their blk entries.
 * Does not include the double indirect block itself.
 */
ssize_t __free_double_indirect_blk(int fd,
						struct uwufs_indirect_blk *double_indirect_blk);

/**
 * Frees all blk entries in a triple indirect block recursively,
 * including all the double and single indirect blocks and
 * their blk entries.
 * Does not include the triple indirect block itself.
 */
ssize_t __free_triple_indirect_blk(int fd,
						struct uwufs_indirect_blk *triple_indirect_blk);

ssize_t remove_file(int fd,
					  struct uwufs_inode *inode,
					  uwufs_blk_t inode_num);
#endif
