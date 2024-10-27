/**
 * 	Author: Joseph
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
	off_t status = lseek(fd, blk_num * UWUFS_BLOCK_SIZE, SEEK_SET);
	if (status < 0) {
#ifdef DEBUG
		perror("read_blk: cannot lseek to blk");
#endif
		return status;
	}
	
	ssize_t bytes_read = read(fd, buf, UWUFS_BLOCK_SIZE);
	if (bytes_read < 0) {
#ifdef DEBUG
		perror("read_blk: error reading from blk");
#endif
		return bytes_read;
	}
	return bytes_read;
}

ssize_t write_blk(int fd,
				  const void* buf,
				  size_t size,
				  uwufs_blk_t blk_num)
{
	off_t status = lseek(fd, blk_num * UWUFS_BLOCK_SIZE, SEEK_SET);
	if (status < 0) {
#ifdef DEBUG
		perror("write_blk: cannot lseek to blk");
#endif
		return status;
	}

	ssize_t bytes_written = write(fd, buf, UWUFS_BLOCK_SIZE);
	if (bytes_written != UWUFS_BLOCK_SIZE) {
#ifdef DEBUG
		perror("write_blk: error writing to blk");
#endif
		return bytes_written;
	}
	return bytes_written;
}

ssize_t read_inode(int fd, void* buf, uwufs_blk_t inode_num)
{
	// TEMP: Should read from superblock of the ilist_start for 
	// 		flexibility/compatability (non default values)
	uwufs_blk_t inode_blk_num = 1 + UWUFS_RESERVED_SPACE; // TEMP: Hard coded
	inode_blk_num += (inode_num * sizeof(struct uwufs_inode))
					  / UWUFS_BLOCK_SIZE;
	
	struct uwufs_inode_blk inode_blk;
	ssize_t status = read_blk(fd, &inode_blk, inode_blk_num);
	if (status < 0) {
#ifdef DEBUG
		perror("read_inode: cannot read inode_blk");
#endif
		return status;
	}

	uwufs_blk_t inode_num_in_blk = (inode_num * sizeof(struct uwufs_inode))
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

	struct uwufs_inode_blk inode_blk;
	ssize_t status = read_blk(fd, &inode_blk, inode_blk_num);
	if (status < 0) {
#ifdef DEBUG
		perror("write_inode: cannot read inode_blk");
#endif
		return status;
	}

	uwufs_blk_t inode_num_in_blk = (inode_num * sizeof(struct uwufs_inode))
									% UWUFS_BLOCK_SIZE;
	inode_num_in_blk /= sizeof(struct uwufs_inode);

	memcpy(&inode_blk.inodes[inode_num_in_blk], buf, size);

	status = write_blk(fd, &inode_blk, UWUFS_BLOCK_SIZE, inode_blk_num);
	if (status < 0) {
#ifdef DEBUG
		perror("write_inode: cannot write inode_blk");
#endif
		return status;
	}
	return status;
}


// Return the first free inode
uwufs_blk_t get_first_free_inode(int fd, 
								uwufs_blk_t ilist_start, 
								uwufs_blk_t ilist_size)
{
    // buffers for inode blk and inode
    struct uwufs_inode_blk inode_blk;
    struct uwufs_inode inode;
    uwufs_blk_t current_inode_blk = ilist_start;
    uint16_t cur_inode_link_count = -1;

    // read one inode block at a time 
    while (cur_inode_link_count != 0 &&
        current_inode_blk < (ilist_start + ilist_size)) 
    {
		ssize_t status = read_blk(fd, &inode_blk, current_inode_blk);
        if (status < 0) {
    #ifdef DEBUG
            perror("read_inode: cannot read inode_blk");
    #endif
            return status;
        }

        int inodes_per_block = (UWUFS_BLOCK_SIZE / UWUFS_INODE_DEFAULT_SIZE);

        // check each inode in the inode block; link count == 0 means it is unallocated
        for (int inode_in_blk = 0; inode_in_blk < inodes_per_block; 
			inode_in_blk++) 
		{
            memcpy(&inode, &inode_blk.inodes[inode_in_blk], sizeof(struct uwufs_inode));
            cur_inode_link_count = inode.link_count;

            // found a free inode
            if (cur_inode_link_count == 0) {
        
                // calculate inode number and return
                uwufs_blk_t inode_num = (current_inode_blk - ilist_start) 
                                        * inodes_per_block + inode_in_blk;
                return inode_num;
            }
        }
        current_inode_blk++;
    }
     
    return EDQUOT; // no free inodes
}

