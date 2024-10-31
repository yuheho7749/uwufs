/**
 * 	Implements file operations for uwufs.
 *
 * 	Author: Joseph
 */

#include "file_operations.h"
#include "uwufs.h"

#include <errno.h>
#include <string.h>

ssize_t create_file(uwufs_blk_t *inode, uwufs_aflags_t flags)
{
	// TODO:
	return -1;
}

ssize_t put_directory_file_entry(struct uwufs_directory_data_blk *dir_blk,
								 const char name[UWUFS_FILE_NAME_SIZE],
								 uwufs_blk_t file_inode_num)
{
	int n = sizeof(struct uwufs_directory_file_entry);
	int i;
	struct uwufs_directory_file_entry file_entry;
	for (i = 0; i < n; i++) {
		file_entry = dir_blk->file_entries[i];
		if (file_entry.inode_num == 0) {
			dir_blk->file_entries[i].inode_num = file_inode_num;
			memcpy(dir_blk->file_entries[i].file_name, name,
				   UWUFS_FILE_NAME_SIZE);
			return 0;
		}
	}

	// NOTE: Only cares if the provided dir_blk has space
	return -ENOSPC;
}
