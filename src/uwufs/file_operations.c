/**
 * Implements file operations for uwufs.
 *
 * Authors: Joseph, Kay
 */

#include "file_operations.h"
#include "low_level_operations.h"
#include "uwufs.h"

#include <errno.h>
#include <string.h>

ssize_t create_file(uwufs_blk_t *inode, uint16_t mode)
{
	// TODO:
	return -1;
}

ssize_t put_directory_file_entry(struct uwufs_directory_data_blk *dir_blk,
								 const char name[UWUFS_FILE_NAME_SIZE],
								 uwufs_blk_t file_inode_num)
{
	struct uwufs_directory_file_entry file_entry;
	int n = UWUFS_BLOCK_SIZE/sizeof(file_entry);
	int i;
	for (i = 0; i < n; i++) {
		file_entry = dir_blk->file_entries[i];
		if (file_entry.inode_num == 0) {
			dir_blk->file_entries[i].inode_num = file_inode_num;
			memcpy(dir_blk->file_entries[i].file_name, name,
				   UWUFS_FILE_NAME_SIZE);
			dir_blk->file_entries[i].file_name[UWUFS_FILE_NAME_SIZE-1] = '\0';
			return 0;
		}
	}

	// NOTE: Only cares if the provided dir_blk has space
	return -ENOSPC;
}


ssize_t add_directory_file_entry(int fd,
								 const uwufs_blk_t dir_inode_num,
								 const char name[UWUFS_FILE_NAME_SIZE],
								 uwufs_blk_t file_inode_num,
								 int nlinks_change)
{
	ssize_t status;

	struct uwufs_inode dir_inode;
	read_inode(fd, &dir_inode, dir_inode_num);

	struct uwufs_directory_data_blk dir_blk;
	int i;
	for (i = 0; i < UWUFS_DIRECT_BLOCKS; i++) {
		uwufs_blk_t dir_blk_num = dir_inode.direct_blks[i];

		// alloc new data blk
		if (dir_blk_num == 0) {
			status = malloc_blk(fd, &dir_blk_num);
			if (status < 0 || dir_blk_num <= 0)
				return -ENOSPC;

			dir_inode.direct_blks[i] = dir_blk_num;
			dir_inode.file_size += UWUFS_BLOCK_SIZE;

			memset(&dir_blk, 0, sizeof(dir_blk));
		}
		else {
			read_blk(fd, &dir_blk, dir_blk_num);
		}

		status = put_directory_file_entry(&dir_blk, name, file_inode_num);
		if (status < 0) // no space, so go to next dir blk
			continue;

		// increase nlinks (if added new dir)
		dir_inode.file_links_count += nlinks_change;
		write_inode(fd, &dir_inode, sizeof(dir_inode), dir_inode_num);

		// not sure if want to keep write in this fn or move out of 
		status = write_blk(fd, &dir_blk, dir_blk_num);
		return 0;
	}

	// NOTE: Only cares if none of the direct blks have space
	// TODO: indirect blks
	return -ENOSPC;
}
