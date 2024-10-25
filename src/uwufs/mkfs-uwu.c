/**
 *	Formats block device in the uwufs format.
 *
 *	Author: Joseph
 */


#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/fs.h>

#include "uwufs.h"

char buf[UWUFS_BLOCK_SIZE];

// TODO: Start with simple linked list
static uwufs_blk_t init_freelist(int fd,
								 uwufs_blk_t blk_dev_size,
								 float ilist_size) 
{
	
	return -1;
}


/**
 * 	Initializes the superblock. Assumes that `init_freelist` was already 
 * 		called and initialized a freelist. The blk address of the
 * 		freelist head will be passed in `first_free_blk`
 */
static void init_superblock(int fd,
							uwufs_blk_t blk_dev_size, 
							uwufs_blk_t reserved_space,
							float ilist_size,
							uwufs_blk_t first_free_blk)
{
	memset(buf, 0, sizeof(buf));
	int status = lseek(fd, 0, SEEK_SET);
	if (status != 0) {
		perror("mkfs.uwu: cannot lseek to beginning to init superblock\n");
		close(fd);
		exit(1);
	}
	
	// Set super block values
	struct uwufs_super_blk super_blk;
	super_blk.total_blks = blk_dev_size / UWUFS_BLOCK_SIZE;
	super_blk.freelist_head = first_free_blk;
	super_blk.ilist_start = 1 + reserved_space; // after super + reserved
	uwufs_blk_t ilist_s = ilist_size * super_blk.total_blks;
	super_blk.ilist_size = ilist_s;
	
	memcpy(buf, &super_blk, sizeof(super_blk));

	// Write super block to device
	ssize_t bytes_written = write(fd, buf, UWUFS_BLOCK_SIZE);
	if (bytes_written != UWUFS_BLOCK_SIZE) {
		perror("mkfs.uwu: error writing superblock\n");
		close(fd);
		exit(1);
	}
}

// TODO:
/**
 * 	Formats the block device to uwufs format.
 *
 * 	`fd`: the opened block device
 * 	`blk_dev_size`: total size of the opened block device
 * 	`reserved_space`: number of blocks reserved after superblock
 * 		and before the start of i-list
 * 	`ilist_size`: percentage of block device size used for i-list
 */
static int init_uwufs(int fd,
					  uwufs_blk_t blk_dev_size,
					  uwufs_blk_t reserved_space,
					  float ilist_size)
{
	// init i-list/i-nodes (nothing to be done)
	
	// init free list
	uwufs_blk_t first_free_blk = init_freelist(fd, blk_dev_size, ilist_size);
	
	init_superblock(fd, blk_dev_size, reserved_space, ilist_size,
				  	first_free_blk);
#ifdef DEBUG
	printf("Initialized super block\n");
#endif
	
	// init root directory

	return 0;
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("Usage: %s [block device]\n", argv[0]);
		return 1;
	}

	int fd = open(argv[1], O_RDWR);
	if (fd < 0) {
		perror("Failed to access block device\n");
		return 1;
	}
	
	int ret = 0;

	// Get and check size of block device
	uwufs_blk_t blk_dev_size;
	ret = ioctl(fd, BLKGETSIZE64, &blk_dev_size);
	if (ret < 0) {
		perror("Not a block device or cannot determine size of device\n");
		close(fd);
		return 1;
	}
#ifdef DEBUG
	printf("Block device %s size: %ld\n", argv[1], blk_dev_size);
#endif

	// NOTE: Read user definable params later.
	// 	Specifing blk_dev_size to format can help with testing too
	ret = init_uwufs(fd, blk_dev_size, UWUFS_RESERVED_SPACE,
				  	 UWUFS_ILIST_DEFAULT_SIZE);

	// printf("Successfully formated device %s\n", argv[1]);
	close(fd);
	return ret;
}
