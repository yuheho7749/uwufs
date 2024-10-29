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

ssize_t malloc_blk(int fd, uwufs_blk_t *blk_num)
{
	// Read super blk for freelist head
	struct uwufs_super_blk super_blk;
	ssize_t status = read_blk(fd, &super_blk, 0);
	if (status < 0) {
#ifdef DEBUG
		perror("malloc_blk: cannot read super_blk");
#endif
		return status;
	}

	uwufs_blk_t freelist_head = super_blk.freelist_head;

	if (freelist_head <= 0) {
		return -ENOSPC;
	}

	// Updated freelist head to point to next free data block
	struct uwufs_free_data_blk free_blk;
	status = read_blk(fd, &free_blk, freelist_head);
	if (status < 0) {
#ifdef DEBUG
		perror("malloc_blk: cannot read freelist_head blk");
#endif
		return status;
	}

	super_blk.freelist_head = free_blk.next_free_blk;
	status = write_blk(fd, &super_blk, UWUFS_BLOCK_SIZE, 0);
	if (status < 0) {
#ifdef DEBUG
		perror("malloc_blk: cannot update freelist_head blk");
#endif
		return status;
	}

	// Return malloc'd block
	*blk_num = freelist_head;
	return 0;
}

ssize_t free_blk(int fd, const uwufs_blk_t blk_num)
{
	struct uwufs_super_blk super_blk;
	ssize_t status = read_blk(fd, &super_blk, 0);
	if (status < 0) {
#ifdef DEBUG
		perror("free_blk: cannot read super_blk");
#endif
		return status;
	}

	struct uwufs_free_data_blk new_freelist_head;
	new_freelist_head.next_free_blk = super_blk.freelist_head;
	super_blk.freelist_head = blk_num;

	status = write_blk(fd, &new_freelist_head, UWUFS_BLOCK_SIZE, blk_num);
	if (status < 0) {
#ifdef DEBUG
		perror("free_blk: cannot update new freelist_head blk");
#endif
		return status;
	}

	status = write_blk(fd, &super_blk, UWUFS_BLOCK_SIZE, 0);
	if (status < 0) {
#ifdef DEBUG
		perror("free_blk: cannot update super_blk");
#endif
		return status;
	}

	return 0;
}
