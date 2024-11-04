/**
 * Author: Joseph
 */

#include "low_level_operations.h"
#include "uwufs.h"

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

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
				  size_t size,
				  uwufs_blk_t blk_num)
{
	ssize_t status = lseek(fd, blk_num * UWUFS_BLOCK_SIZE, SEEK_SET);
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

	status = write_blk(fd, &inode_blk, UWUFS_BLOCK_SIZE, inode_blk_num);
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

	freelist_head = super_blk.freelist_head;

	if (freelist_head <= 0)
		return -ENOSPC;

	// Updated freelist head to point to next free data block
	status = read_blk(fd, &free_blk, freelist_head);
	if (status < 0)
		goto debug_msg_ret;

	super_blk.freelist_head = free_blk.next_free_blk;
	status = write_blk(fd, &super_blk, UWUFS_BLOCK_SIZE, 0);
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

	status = write_blk(fd, &new_freelist_head, UWUFS_BLOCK_SIZE, blk_num);
	if (status < 0)
		goto debug_msg_ret;

	status = write_blk(fd, &super_blk, UWUFS_BLOCK_SIZE, 0);
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
    // read superblk for ilist start & size
	struct uwufs_super_blk super_blk;
	ssize_t status = read_blk(fd, &super_blk, 0);
	if (status < 0)
		goto debug_msg_ret;
	
	// buffers for inode blk and inode
    struct uwufs_inode_blk inode_blk;
    struct uwufs_inode inode;
    uwufs_blk_t current_inode_blk = super_blk.ilist_start;

    // read one inode block at a time 
    while (current_inode_blk < 
			(super_blk.ilist_start + super_blk.ilist_total_size)) 
    {
		status = read_blk(fd, &inode_blk, current_inode_blk);
        if (status < 0) 
			goto debug_msg_ret;

        int inodes_per_block = (UWUFS_BLOCK_SIZE / UWUFS_INODE_DEFAULT_SIZE);

        // check each inode in the inode block
        for (int inode_in_blk = 0; inode_in_blk < inodes_per_block; 
			inode_in_blk++) 
		{
            memcpy(&inode, &inode_blk.inodes[inode_in_blk], 
					sizeof(struct uwufs_inode));

            // found a free inode
            if (inode.access_flags == F_TYPE_FREE) {
        
                // calculate inode number and return
                uwufs_blk_t num = (current_inode_blk - super_blk.ilist_start) 
                                    * inodes_per_block + inode_in_blk;
                *inode_num = num; // assign inode number 
				return 0;
            }
        }
        current_inode_blk++;
    }
    return EDQUOT; // no free inodes

debug_msg_ret:
#ifdef DEBUG
	perror("find_free_inode error");
#endif
	return status;
}


// namei helper - scan the set of data blocks for next inode
// TODO: only scans direct blks for now
ssize_t search_for_next_inode(int fd, char *file_name, struct uwufs_inode* cur_inode) {
	struct uwufs_directory_data_blk dir_data_blk;
	ssize_t status;

	// scan each direct block 
	for (int i = 0; i < UWUFS_DIRECT_BLOCKS; i++) {
		status = read_blk(fd, &dir_data_blk, cur_inode->direct_blks[i]);
		if (status < 0) 
			goto debug_msg_ret;

		int num_entries = UWUFS_BLOCK_SIZE / (UWUFS_FILE_NAME_SIZE + sizeof(uwufs_blk_t));
		
		// compare each entry in the block
		for (int j = 0; j < num_entries; j++) {
			if (dir_data_blk.file_entries[j].inode_num <= 0) {
				continue;
			}
			if (strcmp(dir_data_blk.file_entries[j].file_name, file_name) == 0) {
					return dir_data_blk.file_entries[j].inode_num;
				#ifdef DEBUG	
					printf(f"Resolved %s with inode number %lu\n", file_name, 
						dir_data_blk.file_entries[j].inode_num)
				#endif
			}
		}
	}
	return -1; //not found

debug_msg_ret:
#ifdef DEBUG
	perror("search_for_next_inode error");
#endif
	return status;
}



// * `fd`: block device
// * `path`: null terminated file path
// * `root_dir_inode`: can be optionally provided to save a super blk read
ssize_t namei(int fd, const char *path, const struct uwufs_inode *root_dir_inode) {
	uwufs_blk_t current_inode_number = UWUFS_ROOT_DIR_INODE;
	struct uwufs_inode current_inode; 
	ssize_t status;
	if (root_dir_inode != NULL) {
		memcpy(&current_inode, root_dir_inode, UWUFS_INODE_DEFAULT_SIZE);
	}
	else {
		read_inode(fd, &current_inode, current_inode_number);
	}

	//since strtok can modify the original string, copy it
	char full_path[strlen(path)];
	strncpy(full_path, path, strlen(path)); 

	char *path_segment = strtok(full_path, "/");
	while (path_segment != NULL) {
		if (current_inode.access_flags & F_TYPE_BITS == F_TYPE_REGULAR) {
			path_segment = strtok(NULL, "/");
			
			if (path_segment != NULL) {
				// then we found a regular file 
				// but we haven't reached the end of the full path yet
				// so invalid path
				return -1;
			}
			else {
				return current_inode_number;
			}
		}
		
		else if (current_inode.access_flags & F_TYPE_BITS == F_TYPE_DIRECTORY) {
			// scan the current inode for the path segment
			current_inode_number = search_for_next_inode(fd, path_segment, &current_inode);
			if (current_inode_number == 0) {
				// return some error (couldn't find the path)
			}
			read_inode(fd, &current_inode, current_inode_number);
			path_segment = strtok(NULL, "/");
		}

		// TODO: handle symlink types
	}

	return -1; 

debug_msg_ret:
#ifdef DEBUG
	perror("namei error");
#endif
	return status;
}
