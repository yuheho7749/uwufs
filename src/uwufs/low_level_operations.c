/**
 * Authors: Joseph, Kay
 */

#include "low_level_operations.h"
#include "file_operations.h"
#include "uwufs.h"

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "cpp/c_api.h"

ssize_t read_blk(int fd, void* buf, uwufs_blk_t blk_num)
{
	ssize_t status = lseek(fd, blk_num * UWUFS_BLOCK_SIZE, SEEK_SET);
	if (status < 0)
		goto debug_msg_ret;

	status = read(fd, buf, UWUFS_BLOCK_SIZE);
	if (status < 0)
		goto debug_msg_ret;

	return status;

debug_msg_ret:
#ifdef DEBUG
	perror("read_blk error");
#endif
	return status;
}

ssize_t write_blk(int fd,
				  const void* buf,
				  uwufs_blk_t blk_num)
{

	off_t offset = blk_num * UWUFS_BLOCK_SIZE;
	ssize_t status = lseek(fd, offset, SEEK_SET);
	if (status < 0)
		goto debug_msg_ret;

	status = write(fd, buf, UWUFS_BLOCK_SIZE);
	if (status != UWUFS_BLOCK_SIZE)
		goto debug_msg_ret;

	return status;

debug_msg_ret:
#ifdef DEBUG
	perror("write_blk error");
#endif
	return status;
}

ssize_t read_inode(int fd, void* buf, uwufs_blk_t inode_num)
{
	// TEMP: Should read from superblock of the ilist_start for 
	// 		flexibility/compatability (non default values)
	uwufs_blk_t inode_blk_num = 1 + UWUFS_RESERVED_SPACE; // TEMP: Hard coded
	inode_blk_num += (inode_num * sizeof(struct uwufs_inode))
					  / UWUFS_BLOCK_SIZE;

	uwufs_blk_t inode_num_in_blk;
	struct uwufs_inode_blk inode_blk;
	ssize_t status = read_blk(fd, &inode_blk, inode_blk_num);
	if (status < 0) {
#ifdef DEBUG
		perror("read_inode: cannot read inode_blk");
#endif
		return status;
	}

	inode_num_in_blk = (inode_num * sizeof(struct uwufs_inode))
									% UWUFS_BLOCK_SIZE;
	inode_num_in_blk /= sizeof(struct uwufs_inode);
	memcpy(buf, &inode_blk.inodes[inode_num_in_blk], sizeof(struct uwufs_inode));

	return sizeof(struct uwufs_inode);
}

ssize_t write_inode(int fd,
					const void* buf,
					size_t size,
					uwufs_blk_t inode_num)
{
	// TEMP: Should read from superblock of the ilist_start for 
	// 		flexibility/compatability (non default values)
	uwufs_blk_t inode_blk_num = 1 + UWUFS_RESERVED_SPACE; // TEMP: Hard coded
	inode_blk_num += (inode_num * sizeof(struct uwufs_inode))
					  / UWUFS_BLOCK_SIZE;

	uwufs_blk_t inode_num_in_blk;
	struct uwufs_inode_blk inode_blk;
	ssize_t status = read_blk(fd, &inode_blk, inode_blk_num);
	if (status < 0)
		goto debug_msg_ret;

	inode_num_in_blk = (inode_num * sizeof(struct uwufs_inode))
									% UWUFS_BLOCK_SIZE;
	inode_num_in_blk /= sizeof(struct uwufs_inode);

	memcpy(&inode_blk.inodes[inode_num_in_blk], buf, size);

	status = write_blk(fd, &inode_blk, inode_blk_num);
	if (status < 0)
		goto debug_msg_ret;

	return status;

debug_msg_ret:
#ifdef DEBUG
	perror("write_inode error");
#endif
	return status;
}

ssize_t malloc_blk(int fd, uwufs_blk_t *blk_num)
{
	// Read super blk for freelist head
	struct uwufs_super_blk super_blk;
	struct uwufs_free_data_blk free_blk;
	uwufs_blk_t freelist_head;

	ssize_t status = read_blk(fd, &super_blk, 0);
	if (status < 0)
		goto debug_msg_ret;

	if (super_blk.free_blks_left == 0)
		return -ENOSPC;

	freelist_head = super_blk.freelist_head;

	if (freelist_head <= 0)
		return -ENOSPC;

	// Updated freelist head to point to next free data block
	status = read_blk(fd, &free_blk, freelist_head);
	if (status < 0)
		goto debug_msg_ret;

	super_blk.freelist_head = free_blk.next_free_blk;
	super_blk.free_blks_left -= 1;
	status = write_blk(fd, &super_blk, 0);
	if (status < 0)
		goto debug_msg_ret;

	// Return malloc'd block
	*blk_num = freelist_head;
	return 0;

debug_msg_ret:
#ifdef DEBUG
	perror("malloc_blk error");
#endif
	return status;
}

ssize_t free_blk(int fd, const uwufs_blk_t blk_num)
{
	struct uwufs_super_blk super_blk;
	struct uwufs_free_data_blk new_freelist_head;
	ssize_t status = read_blk(fd, &super_blk, 0);
	if (status < 0)
		goto debug_msg_ret;

	new_freelist_head.next_free_blk = super_blk.freelist_head;
	super_blk.freelist_head = blk_num;
	super_blk.free_blks_left += 1;

	status = write_blk(fd, &new_freelist_head, blk_num);
	if (status < 0)
		goto debug_msg_ret;

	status = write_blk(fd, &super_blk, 0);
	if (status < 0)
		goto debug_msg_ret;

	return 0;

debug_msg_ret:
#ifdef DEBUG
	perror("free_blk error");
#endif
	return status;
}


ssize_t find_free_inode(int fd, uwufs_blk_t *inode_num) {
    // buffers for inode blk and inode
    struct uwufs_inode_blk inode_blk;
    struct uwufs_inode inode;
    uwufs_blk_t current_inode_blk;
	
	// read superblk for ilist start & size
	struct uwufs_super_blk super_blk;
	ssize_t status = read_blk(fd, &super_blk, 0);
	if (status < 0)
		goto debug_msg_ret;

	// if (super_blk.free_inodes_left == 0)
	// 	return -ENOSPC;
	current_inode_blk = super_blk.ilist_start;

    // read one inode block at a time 
    while (current_inode_blk < 
			(super_blk.ilist_start + super_blk.ilist_total_size)) 
    {
		status = read_blk(fd, &inode_blk, current_inode_blk);
        if (status < 0) 
			goto debug_msg_ret;

        int inodes_per_block = (UWUFS_BLOCK_SIZE / sizeof(struct uwufs_inode));

        // check each inode in the inode block
        for (int inode_in_blk = 0; inode_in_blk < inodes_per_block; 
			inode_in_blk++) 
		{
			memcpy(&inode, &inode_blk.inodes[inode_in_blk], 
					sizeof(struct uwufs_inode));

            // found a free inode
            if ((inode.file_mode & F_TYPE_BITS) == F_TYPE_FREE) {
        
                // calculate inode number and return
                uwufs_blk_t num = (current_inode_blk - super_blk.ilist_start)
                                    * inodes_per_block + inode_in_blk;
				if (num < UWUFS_ROOT_DIR_INODE) {
					continue; // don't assign inode numbers <0-1>
				}
                *inode_num = num; // assign inode number 
#ifdef DEBUG
				printf("Found free inode: %lu\n", *inode_num);
#endif
				return 0;
            }
        }
        current_inode_blk++;
    }
    return -EDQUOT; // no free inodes

debug_msg_ret:
#ifdef DEBUG
	perror("find_free_inode error");
#endif
	return status;
}


ssize_t next_inode_in_path(int fd,
						   char *file_name, 
						   struct uwufs_inode* cur_inode,
						   uwufs_blk_t *inode_num) {

	struct uwufs_directory_data_blk dir_data_blk;
	uwufs_blk_t dir_data_blk_num;
	ssize_t status;
	int num_entries = UWUFS_BLOCK_SIZE / sizeof(struct uwufs_directory_file_entry);
	int n = (cur_inode->file_size + UWUFS_BLOCK_SIZE - 1) / UWUFS_BLOCK_SIZE;

	dblk_itr_t dblk_itr = create_dblk_itr(cur_inode, fd, 0);

	int i, j;
	for (i = 0; i < n; i++) {
		dir_data_blk_num = dblk_itr_next(dblk_itr);
		if (dir_data_blk_num == 0) {
			status = -EIO;
			goto debug_msg_ret;
		}
		status = read_blk(fd, &dir_data_blk, dir_data_blk_num);
		if (status < 0) 
			goto debug_msg_ret;

		for (j = 0; j < num_entries; j++) {
			if (dir_data_blk.file_entries[j].inode_num <= 0) {
				continue;
			}
			if (strlen(file_name) > 0 &&
				strcmp(dir_data_blk.file_entries[j].file_name, file_name) == 0) {
				*inode_num = dir_data_blk.file_entries[j].inode_num;
#ifdef DEBUG
				// printf("\t\tResolved %s with inode number %lu\n", file_name,
				// 	dir_data_blk.file_entries[j].inode_num);
#endif
				destroy_dblk_itr(dblk_itr);
				return 0;
			}
		}
	}
	destroy_dblk_itr(dblk_itr);
	return -ENOENT; //not found

debug_msg_ret:
	destroy_dblk_itr(dblk_itr);
#ifdef DEBUG
	perror("search_for_next_inode error");
#endif
	return status;
}


ssize_t namei(int fd, 
 			  const char *path, 
			  const struct uwufs_inode *root_dir_inode,
			  uwufs_blk_t *inode_num) {

	uwufs_blk_t current_inode_number = UWUFS_ROOT_DIR_INODE;
	struct uwufs_inode current_inode; 
	ssize_t status;

	if (root_dir_inode != NULL) 
		memcpy(&current_inode, root_dir_inode, sizeof(current_inode));
	else {
		status = read_inode(fd, &current_inode, current_inode_number);
		if (status < 0) {
			perror("Couldn't read root dir inode");
			return status;
		}
	}

	//since strtok can modify the original string, copy it
	char full_path[strlen(path)];
	strncpy(full_path, path, strlen(path)); 
	full_path[strlen(path)] = '\0';
	char* strtok_ptr;
	char *path_segment;

	path_segment = strtok_r(full_path, "/", &strtok_ptr);
	while (path_segment != NULL) {
		status = 0;
		// check if path segment is < UWUFS_FILE_NAME_SIZE
		if (strlen(path_segment) >= UWUFS_FILE_NAME_SIZE) {
			printf("Path segment exceeds FILE_NAME_SIZE\n");
			return -ENAMETOOLONG;
		}

		// regular file
		if ((current_inode.file_mode & F_TYPE_BITS) == F_TYPE_REGULAR) {
			path_segment = strtok_r(NULL, "/", &strtok_ptr);
			
			if (path_segment != NULL) 
				return -ENOENT; // regular file found but not at leaf of path
			break;
		}
		// directory
		else if ((current_inode.file_mode & F_TYPE_BITS) == F_TYPE_DIRECTORY) {
			status = next_inode_in_path(fd, path_segment, &current_inode,
										&current_inode_number);
			if (status == -ENOENT) // File does not exist
				return status;
		}
		// symlink
		// NOTE: Use readlink syscall later
		// Also need to handle other file types in the future
		else {
			struct uwufs_regular_file_data_blk symlink_data;
			// assumes symlink contents stored in first direct blk
			read_blk(fd, &symlink_data, current_inode.direct_blks[0]); 

			char symlink_path[strlen(symlink_data.data)];
			strncpy(symlink_path, symlink_data.data, strlen(symlink_data.data)); 

			// start following the new path in the symlink
			// TODO: maybe add an argument to namei() call that counts the # symlink
			// 		to avoid recursively dereferencing more than MAX_SYMLINK times
			status = namei(fd, symlink_path, root_dir_inode, &current_inode_number);
		}

		// have next inode num so read in next block & get next path segment
		if (status < 0 || current_inode_number == 0)
			goto debug_msg_ret;
		status = read_inode(fd, &current_inode, current_inode_number);
		if (status < 0)
			goto debug_msg_ret;
		path_segment = strtok_r(NULL, "/", &strtok_ptr);
	}

	*inode_num = current_inode_number;
	return 0;

debug_msg_ret:
#ifdef DEBUG
	perror("namei error");
#endif
	return status;
}
