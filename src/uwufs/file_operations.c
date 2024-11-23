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

#include "cpp/c_api.h"

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
	struct uwufs_directory_data_blk dir_data_blk;
	uwufs_blk_t dir_data_blk_num;

	unix_time = time(NULL);
	if (unix_time == -1)
		unix_time = 0;

	struct uwufs_inode dir_inode;
	status = read_inode(fd, &dir_inode, dir_inode_num);
	RETURN_IF_ERROR(status);

	// ceil division although it is not necessary if size of dir is
	// UWUFS_BLOCK_SIZE aligned
	uwufs_blk_t n = (dir_inode.file_size + UWUFS_BLOCK_SIZE - 1)
		/ UWUFS_BLOCK_SIZE;

	// If directory is completely empty
	if (n == 0) {
		status = malloc_blk(fd, &dir_data_blk_num);
		if (status < 0 || dir_data_blk_num <= 0)
			return -ENOSPC;

		dir_inode.direct_blks[0] = dir_data_blk_num;
		dir_inode.file_size += UWUFS_BLOCK_SIZE;
		dir_inode.file_ctime = (uint64_t)unix_time;

		memset(&dir_data_blk, 0, sizeof(dir_data_blk));
	} else { // Read last block
		// Assume the dir entries are packed (no holes)
		dir_data_blk_num = get_dblk(&dir_inode, fd, n-1);
		if (dir_data_blk_num == 0)
			return -EIO;
	}

	status = read_blk(fd, &dir_data_blk, dir_data_blk_num);
	RETURN_IF_ERROR(status);

	status = put_directory_file_entry(&dir_data_blk, name, file_inode_num);
	if (status == -ENOSPC) {
		status = malloc_blk(fd, &dir_data_blk_num);
		if (status < 0 || dir_data_blk_num <= 0)
			return -ENOSPC;
		dir_inode.file_size += UWUFS_BLOCK_SIZE;
		dir_inode.file_ctime = (uint64_t)unix_time;

		memset(&dir_data_blk, 0, sizeof(dir_data_blk));
		status = put_directory_file_entry(&dir_data_blk, name, file_inode_num);
		if (status < 0) return status;
		uwufs_blk_t dir_data_blk_num2 = append_dblk(&dir_inode, fd, n,
										  dir_data_blk_num);
#ifdef DEBUG
		assert(dir_data_blk_num2 == dir_data_blk_num);
#endif
	} else if (status < 0) {
		return status;
	}

	// Entry add success and return
	dir_inode.file_links_count += nlinks_change;
	if (nlinks_change != 0)
		dir_inode.file_ctime = (uint64_t)unix_time;
	dir_inode.file_mtime = (uint64_t)unix_time;
	dir_inode.file_atime = (uint64_t)unix_time;

	status = write_inode(fd, &dir_inode, sizeof(dir_inode), dir_inode_num);
	RETURN_IF_ERROR(status);

	status = write_blk(fd, &dir_data_blk, dir_data_blk_num);
	RETURN_IF_ERROR(status);
	return 0;
}

// NOTE: Deprecated (need to handle indirect blocks)
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
	// NOTE: indirect blocks
	*nentries = num_entries;
	return 0;
}

// Assumes the directory entries are semi-packed, meaning
// 		entries are packed in the lowest number of data blocks
// 		except for the last data block, which may be not completely
// 		filled. The last data block does not have to be strictly packed
bool is_directory_empty(int fd, struct uwufs_inode *dir_inode) {
	ssize_t status;
	size_t i;
	int count = 0;

	struct uwufs_directory_data_blk dir_blk;
	status = read_blk(fd, &dir_blk, dir_inode->direct_blks[0]);
	RETURN_IF_ERROR(status);

	for (i = 0; i < UWUFS_BLOCK_SIZE/sizeof(struct uwufs_directory_data_blk); i++) {
		if (dir_blk.file_entries[i].inode_num != 0) {
			count += 1;
		}
	}
	return count <= 2;
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
						 struct uwufs_directory_data_blk *dir_data_blk,
						 struct uwufs_directory_data_blk *last_dir_data_blk,
						 int last_file_entry_index,
						 const char name[UWUFS_FILE_NAME_SIZE],
						 uwufs_blk_t file_inode_num)
{
	ssize_t status;
	int i;
	int n = UWUFS_BLOCK_SIZE/sizeof(struct uwufs_directory_file_entry);
	struct uwufs_directory_file_entry file_entry;

	for (i = 0; i < n; i++) {
		file_entry = dir_data_blk->file_entries[i];
		if (file_entry.inode_num == file_inode_num &&
			(name == NULL || strcmp(file_entry.file_name, name) == 0)) {
			// clear file entry
			goto found_entry_ret;
		}
	}
	return -ENOENT;

found_entry_ret:
	if (last_dir_data_blk != NULL) { // move the last entry to replace the one removed
		printf("swap and erase\n");
		memcpy(&dir_data_blk->file_entries[i],
			 &(last_dir_data_blk->file_entries[last_file_entry_index]),
			 sizeof(file_entry));
		memset(&(last_dir_data_blk->file_entries[last_file_entry_index]),
			 0, sizeof(file_entry));
	} else {
		printf("just erase\n");
		memset(&dir_data_blk->file_entries[i], 0, sizeof(file_entry));
	}

	// 0 or positive when successful - can be used to tell if last dir
	// is empty assuming the last_file_entry_index is scanned in reverse
	// by the caller of this function
	return last_file_entry_index;
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
	uwufs_blk_t i;
	uwufs_blk_t last_dblk_num;
	uwufs_blk_t last_dblk_index;
	struct uwufs_directory_data_blk dir_data_blk;
	uwufs_blk_t dir_data_blk_num;
	struct uwufs_directory_data_blk last_dir_data_blk;
	uwufs_blk_t num_data_blks;
	uwufs_blk_t last_dir_entry_inode_num;
	int last_file_entry_index;
	int num_file_entries = UWUFS_BLOCK_SIZE / sizeof(struct uwufs_directory_file_entry);
	dblk_itr_t dblk_itr = NULL;

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

	// Find last block for compacting things
	num_data_blks = (parent_inode.file_size + UWUFS_BLOCK_SIZE - 1)
		/ UWUFS_BLOCK_SIZE;
	last_dblk_num = get_dblk(&parent_inode, fd, num_data_blks - 1);
	status = read_blk(fd, &last_dir_data_blk, last_dblk_num);
	if (status < 0) return status;
	for (last_file_entry_index = num_file_entries - 1;
		last_file_entry_index >= 0;
		last_file_entry_index--) {
		last_dir_entry_inode_num = last_dir_data_blk.file_entries[last_file_entry_index].inode_num;
		printf("last_file_entry_index %d\n", last_file_entry_index);
		printf("last_dir_entry_inode_num %lu\n", last_dir_entry_inode_num);
		if (last_dir_entry_inode_num > 0) {
			goto found_last_entry;
		}
	}
	printf("unlink_file: directory data block has inconsistent entries\n");
	return -EIO;

found_last_entry:
	dblk_itr = create_dblk_itr(&parent_inode, fd, 0);
	for (i = 0; i < num_data_blks; i++) {
		dir_data_blk_num = dblk_itr_next(dblk_itr);
		if (dir_data_blk_num == 0) {
			status = -ENOENT;
			goto fail_ret;
		}

		status = read_blk(fd, &dir_data_blk, dir_data_blk_num);
		if (status < 0)
			goto fail_ret;

		printf("last_file_entry_index: %d\n", last_file_entry_index);
		// this looks bad, but compiler will optimizes it :)
		if (last_dblk_num == dir_data_blk_num) {
			status = __remove_entry_from_dir_data_blk(fd,
												&dir_data_blk,
												NULL,
												last_file_entry_index,
												child_path,
												inode_num);
		} else {
			status = __remove_entry_from_dir_data_blk(fd,
												&dir_data_blk,
												&last_dir_data_blk,
												last_file_entry_index,
												child_path,
												inode_num);
		}

		printf("unlink_file 2\n");
		printf("remove entry status %ld\n", status);
		if (status == -ENOENT)
			continue;
		if (status < 0)
			goto fail_ret;

		// last file entry index is 0 so the last blk is empty
		if (status == 0) {
			printf("free blk\n");
			last_dblk_index = remove_dblk(&parent_inode, fd, num_data_blks - 1);
			printf("last_dblk_index %lu\n", last_dblk_index);
			printf("num_data_blks %lu\n", num_data_blks);
			printf("last_dblk_num %lu\n", last_dblk_num);
			for (int t = 0; t < 10; t++) {
				printf("t: %lu", parent_inode.direct_blks[t]);
			}
			if (last_dblk_index != last_dblk_num) {
				status = -EIO;
				goto fail_ret;
			}
			parent_inode.file_size -= UWUFS_BLOCK_SIZE;
			parent_inode.file_ctime = (uint64_t)unix_time;
		} else {
			printf("just remove entry\n");
			status = write_blk(fd, &last_dir_data_blk, last_dblk_num);
			if (status < 0) goto fail_ret;
		}

		printf("unlink_file 3\n");
		status = write_blk(fd, &dir_data_blk, dir_data_blk_num);
		if (status < 0)
			goto fail_ret;

		goto success_ret;
	}
fail_ret:
	destroy_dblk_itr(dblk_itr);
	return status;

success_ret:
	destroy_dblk_itr(dblk_itr);
#ifdef DEBUG
	assert(inode->file_links_count >= 1);
#endif
	inode->file_links_count -= 1;
	inode->file_ctime = (uint64_t)unix_time;
	parent_inode.file_links_count += nlinks_change; //unlinking subdir
	parent_inode.file_atime = (uint64_t)unix_time;
	parent_inode.file_mtime = (uint64_t)unix_time;
	status = write_inode(fd, &parent_inode, sizeof(parent_inode),
					  parent_inode_num);
	if (status < 0) return status;
	return 0;
}

ssize_t __free_indirect_blk(int fd,
							struct uwufs_indirect_blk *indirect_blk)
{
	ssize_t status;
	int i;
	int n = UWUFS_BLOCK_SIZE / sizeof(uwufs_blk_t);
	for (i = 0; i < n; i++) {
		if (indirect_blk->entries[i] <= 1 + UWUFS_RESERVED_SPACE)
			continue;
		status = free_blk(fd, indirect_blk->entries[i]);
		if (status < 0) return status;
	}
	return 0;
}

ssize_t __free_double_indirect_blk(int fd,
							struct uwufs_indirect_blk *double_indirect_blk)
{
	ssize_t status;
	struct uwufs_indirect_blk single_indirect_blk;
	int i;
	int n = UWUFS_BLOCK_SIZE / sizeof(uwufs_blk_t);
	for (i = 0; i < n; i++) {
		if (double_indirect_blk->entries[i] <= 1 + UWUFS_RESERVED_SPACE)
			continue;
		status = read_blk(fd, &single_indirect_blk, double_indirect_blk->entries[i]);
		if (status < 0) return status;

		status = __free_indirect_blk(fd, &single_indirect_blk);
		if (status < 0) return status;

		status = free_blk(fd, double_indirect_blk->entries[i]);
		if (status < 0) return status;
	}
	return 0;
}

ssize_t __free_triple_indirect_blk(int fd,
							struct uwufs_indirect_blk *triple_indirect_blk)
{
	ssize_t status;
	struct uwufs_indirect_blk double_indirect_blk;
	int i;
	int n = UWUFS_BLOCK_SIZE / sizeof(uwufs_blk_t);
	for (i = 0; i < n; i++) {
		if (triple_indirect_blk->entries[i] <= 1 + UWUFS_RESERVED_SPACE)
			continue;
		status = read_blk(fd, &double_indirect_blk, triple_indirect_blk->entries[i]);
		if (status < 0) return status;

		status = __free_double_indirect_blk(fd, &double_indirect_blk);
		if (status < 0) return status;

		status = free_blk(fd, triple_indirect_blk->entries[i]);
		if (status < 0) return status;
	}
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
	struct uwufs_indirect_blk single_indirect_blk;
	struct uwufs_indirect_blk double_indirect_blk;
	struct uwufs_indirect_blk triple_indirect_blk;
	int i;

free_triple_indirect_blks:
	if (inode->triple_indirect_blks < 1 + UWUFS_RESERVED_SPACE)
		goto free_double_indirect_blks;
	status = read_blk(fd, &triple_indirect_blk, inode->triple_indirect_blks);
	if (status < 0) return status;
	status = __free_triple_indirect_blk(fd, &triple_indirect_blk);
	if (status < 0) return status;
	status = free_blk(fd, inode->triple_indirect_blks);
	if (status < 0) return status;

free_double_indirect_blks:
	if (inode->double_indirect_blks < 1 + UWUFS_RESERVED_SPACE)
		goto free_single_indirect_blks;
	status = read_blk(fd, &double_indirect_blk, inode->double_indirect_blks);
	if (status < 0) return status;
	status = __free_double_indirect_blk(fd, &double_indirect_blk);
	if (status < 0) return status;
	status = free_blk(fd, inode->double_indirect_blks);
	if (status < 0) return status;

free_single_indirect_blks:
	if (inode->single_indirect_blks < 1 + UWUFS_RESERVED_SPACE)
		goto free_direct_blks;
	status = read_blk(fd, &single_indirect_blk, inode->single_indirect_blks);
	if (status < 0) return status;
	status = __free_indirect_blk(fd, &single_indirect_blk);
	if (status < 0) return status;
	status = free_blk(fd, inode->single_indirect_blks);
	if (status < 0) return status;

free_direct_blks:
	for (i = 0; i < UWUFS_DIRECT_BLOCKS; i++) {
		if (inode->direct_blks[i] < 1 + UWUFS_RESERVED_SPACE)
			continue;
		status = free_blk(fd, inode->direct_blks[i]);
		if (status < 0) return status;
	}

	// NOTE: 2 options
	// 1. mark as free inode
	// 2. clear entire inode
	inode->file_mode = F_TYPE_FREE;
	// memset(inode, 0, sizeof(*inode));
	return 0;
}
