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
#include <stdio.h>
#include <time.h>
#include <stdbool.h>

#ifdef DEBUG
#include <assert.h>
#endif

ssize_t create_file(uwufs_blk_t *inode, uint16_t mode)
{
	// NOTE: Might replace this with __create_regular_file from syscall.c
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
	time_t unix_time;
	unix_time = time(NULL);
	if (unix_time == -1)
		unix_time = 0;

	struct uwufs_inode dir_inode;
	status = read_inode(fd, &dir_inode, dir_inode_num);
	RETURN_IF_ERROR(status);

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
			dir_inode.file_ctime = (uint64_t)unix_time;

			memset(&dir_blk, 0, sizeof(dir_blk));
		}
		else {
			status = read_blk(fd, &dir_blk, dir_blk_num);
			RETURN_IF_ERROR(status);
		}

		status = put_directory_file_entry(&dir_blk, name, file_inode_num);
		if (status < 0) // no space, so go to next dir blk
			continue;

		// increase nlinks (if added new dir)
		dir_inode.file_links_count += nlinks_change;
		if (nlinks_change != 0)
			dir_inode.file_ctime = (uint64_t)unix_time;
		dir_inode.file_mtime = (uint64_t)unix_time;
		dir_inode.file_atime = (uint64_t)unix_time;

		status = write_inode(fd, &dir_inode, sizeof(dir_inode), dir_inode_num);
		RETURN_IF_ERROR(status);

		// not sure if want to keep write in this fn or move out of 
		status = write_blk(fd, &dir_blk, dir_blk_num);
		RETURN_IF_ERROR(status);
		return 0;
	}

	// NOTE: Only cares if none of the direct blks have space
	// TODO: indirect blks
	return -ENOSPC;
}

ssize_t count_directory_file_entries(int fd,
								 	 struct uwufs_inode *dir_inode,
								 	 int *nentries)
{
	ssize_t status;

	struct uwufs_directory_data_blk dir_blk;
	struct uwufs_directory_file_entry file_entry;
	int i;
	int j;
	int n = UWUFS_BLOCK_SIZE/sizeof(file_entry);
	int num_entries = 0;

	for (i = 0; i < UWUFS_DIRECT_BLOCKS; i++) {
		uwufs_blk_t dir_blk_num = dir_inode->direct_blks[i];
		if (dir_blk_num == 0) {
			continue;
		}
		status = read_blk(fd, &dir_blk, dir_blk_num);
		RETURN_IF_ERROR(status);

		for (j = 0; j < n; j++) {
			file_entry = dir_blk.file_entries[j];
			if (file_entry.inode_num != 0) {
				num_entries++;
			}		
		}
	}
	*nentries = num_entries;
	return 0;
}

ssize_t split_path_parent_child(const char *path,
								char *parent_path,
								char *child_dir)
{
    char path_copy[strlen(path)+1];
	strcpy(path_copy, path);
	char *last_dir = strrchr(path_copy, '/');

    if (last_dir != NULL) {
        size_t length = last_dir - path_copy;
        strncpy(parent_path, path_copy, length);
        parent_path[length] = '\0';

		size_t child_dir_length = strlen(last_dir+1); // start after '/'
		if (child_dir_length >= UWUFS_FILE_NAME_SIZE) {
			strncpy(child_dir, last_dir+1, UWUFS_FILE_NAME_SIZE);
			child_dir[UWUFS_FILE_NAME_SIZE-1] = '\0';
			return -ENAMETOOLONG;
		}
		strncpy(child_dir, last_dir+1, child_dir_length);
		child_dir[child_dir_length] = '\0';
		return 0;
    } else {
        // if no '/' is found, error
		// paths should all start from root (?)
#ifdef DEBUG
		printf("Error with split_path_parent_child() function\n");
#endif
		return -ENOENT;
    }
}

ssize_t __remove_entry_from_dir_data_blk(int fd,
										 uwufs_blk_t dir_data_blk_num,
										 const char name[UWUFS_FILE_NAME_SIZE],
										 uwufs_blk_t file_inode_num)
{
	ssize_t status;
	int i;
	int entries = 0;
	bool has_not_removed = true;
	int n = UWUFS_BLOCK_SIZE/sizeof(struct uwufs_directory_file_entry);
	struct uwufs_directory_data_blk dir_data_blk;
	struct uwufs_directory_file_entry file_entry;

	status = read_blk(fd, &dir_data_blk, dir_data_blk_num);
	if (status < 0)
		return status;

	for (i = 0; i < n; i++) {
		file_entry = dir_data_blk.file_entries[i];
		if (file_entry.inode_num != 0)
			entries += 1;
		if (has_not_removed &&
			file_entry.inode_num == file_inode_num &&
			strcmp(file_entry.file_name, name) == 0) {
			has_not_removed = false;
			// clear file entry
			memset(&dir_data_blk.file_entries[i], 0, sizeof(file_entry));
			status = write_blk(fd, &dir_data_blk, dir_data_blk_num);
			if (status < 0) return status;
		}
	}
	if (has_not_removed)
		return -ENOENT;

	return entries - 1;
}

/**
 * Removes directory entry
 */
ssize_t unlink_file(int fd,
					  const char *path,
					  struct uwufs_inode *inode,
					  uwufs_blk_t inode_num,
					  int nlinks_change)
{
	ssize_t status;
	time_t unix_time;
	struct uwufs_inode parent_inode;
	uwufs_blk_t parent_inode_num;
	char parent_path[strlen(path)+1];
	char child_path[UWUFS_FILE_NAME_SIZE];
	int i;
	uwufs_blk_t dir_blk_num;

	unix_time = time(NULL);
	if (unix_time == -1)
		unix_time = 0;

	status = split_path_parent_child(path, parent_path, child_path);
	if (status < 0)
		return status;

	status = namei(fd, parent_path, NULL, &parent_inode_num);
	if (status < 0)
		return -ENOENT;

	status = read_inode(fd, &parent_inode, parent_inode_num);
	if (status < 0)
		return status;

	for (i = 0; i < UWUFS_DIRECT_BLOCKS; i++) {
		dir_blk_num = parent_inode.direct_blks[i];

		if (dir_blk_num == 0)
			continue;

		status = __remove_entry_from_dir_data_blk(fd, dir_blk_num, child_path,
											inode_num);
		if (status == -ENOENT)
			continue;
		if (status < 0)
			return status;

		// if there are no more entries in dir_data_blk, free it
		if (status == 0) {
			status = free_blk(fd, dir_blk_num);
			// NOTE: might compact the blks later
			parent_inode.direct_blks[i] = 0;
			parent_inode.file_size -= UWUFS_BLOCK_SIZE;
			parent_inode.file_ctime = (uint64_t)unix_time;
		}

		goto success_ret;
	}
	// TODO: Indirect blocks

	return -ENOENT;

success_ret:
#ifdef DEBUG
	assert(inode->file_links_count >= 1);
#endif
	inode->file_links_count -= 1;
	inode->file_ctime = (uint64_t)unix_time;
	parent_inode.file_links_count -= nlinks_change; //unlinking subdir
	parent_inode.file_atime = (uint64_t)unix_time;
	parent_inode.file_mtime = (uint64_t)unix_time;
	status = write_inode(fd, &parent_inode, sizeof(parent_inode),
					  parent_inode_num);
	if (status < 0) return status;
	return 0;
}
/**
 * Clears the actual inode/file if the link count is 0
 */
ssize_t remove_file(int fd,
					  struct uwufs_inode *inode,
					  uwufs_blk_t inode_num)
{
	ssize_t status;
	int i;
	for (i = 0; i < UWUFS_DIRECT_BLOCKS; i++) {
		// TEMP: Check if it points to data blks (after ilist)
		if (inode->direct_blks[i] <= 1 + UWUFS_RESERVED_SPACE)
			continue;
		status = free_blk(fd, inode->direct_blks[i]);
		if (status < 0) return status;
	}

	// TODO: Indirect blocks

	// NOTE: 2 options
	// 1. mark as free inode
	// 2. clear entire inode
	inode->file_mode = F_TYPE_FREE;
	// memset(inode, 0, sizeof(*inode));
	return 0;
}
