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
#include "low_level_operations.h"

char buf[UWUFS_BLOCK_SIZE];

/**
 * 	TEMP: Creates a linked list structure for the freelist
 * 
 * Need to use a more efficient method of storing a freelist
 */
static uwufs_blk_t init_freelist(int fd,
								 uwufs_blk_t total_blks,
								 uwufs_blk_t reserved_space,
								 float ilist_percent)
{
	uwufs_blk_t freelist_start = 1 + reserved_space + 
								 (ilist_percent * total_blks);

	// Connect the blocks in the free lists
	uwufs_blk_t current_blk = freelist_start;
	uwufs_blk_t next_blk = current_blk + 1;
	while (next_blk < total_blks) {
		// int status = lseek(fd, current_blk, SEEK_SET);
		// if (status < 0) {
		// 	perror("Abort: failed lseek freelist");
		// 	close(fd);
		// 	exit(1);
		// }
		
		// Connect to next block
		memset(buf, 0, sizeof(buf)); // technically not needed
		memset(buf, next_blk, sizeof(next_blk));

		// Write block to device
		ssize_t bytes_written = write_blk(fd, buf, UWUFS_BLOCK_SIZE,
										  current_blk);
		if (bytes_written != UWUFS_BLOCK_SIZE) {
			perror("Abort: failed writing freelist");
			close(fd);
			exit(1);
		}

		// ssize_t bytes_written = write(fd, buf, UWUFS_BLOCK_SIZE);
		// if (bytes_written != UWUFS_BLOCK_SIZE) {
		// 	perror("Abort: failed writing freelist");
		// 	close(fd);
		// 	exit(1);
		// }
		current_blk = next_blk;
		next_blk ++;
	}

	// Write 0 to last blk
	int status = lseek(fd, current_blk, SEEK_SET);
	if (status < 0) {
		perror("Abort: failed lseek freelist");
		close(fd);
		exit(1);
	}
	memset(buf, 0, sizeof(buf));
	ssize_t bytes_written = write(fd, buf, UWUFS_BLOCK_SIZE);
	if (bytes_written != UWUFS_BLOCK_SIZE) {
		perror("Abort: failed writing freelist");
		close(fd);
		exit(1);
	}
	
	return freelist_start;
}

/**
 * 	Initializes the superblock. Assumes that `init_freelist` was already 
 * 		called and had initialized a freelist. The blk address of the
 * 		freelist head will be passed in `first_free_blk`
 */
static void init_superblock(int fd,
							uwufs_blk_t total_blks, 
							uwufs_blk_t reserved_space,
							float ilist_percent,
							uwufs_blk_t first_free_blk)
{
	memset(buf, 0, sizeof(buf));
	int status = lseek(fd, 0, SEEK_SET);
	if (status != 0) {
		perror("mkfs.uwu: cannot lseek to beginning to init superblock");
		close(fd);
		exit(1);
	}
	
	// Set super block values
	struct uwufs_super_blk super_blk;
	super_blk.total_blks = total_blks;
	super_blk.freelist_head = first_free_blk;
	super_blk.ilist_start = 1 + reserved_space; // after super + reserved
	super_blk.ilist_size = ilist_percent * super_blk.total_blks;	

	memcpy(buf, &super_blk, sizeof(super_blk));

	// Write super block to device
	ssize_t bytes_written = write(fd, buf, UWUFS_BLOCK_SIZE);
	if (bytes_written != UWUFS_BLOCK_SIZE) {
		perror("mkfs.uwu: error writing superblock");
		close(fd);
		exit(1);
	}
}

// TODO:
static void init_root_directory(int fd,
								uwufs_blk_t ilist_start)
{
	int status = lseek(fd, ilist_start, SEEK_SET);
	if (status < 0) {
		perror("mkfs.uwu: cannot lseek to root inode block");
		close(fd);
		exit(1);
	}

	struct uwufs_inode_blk root_inode_blk;
	struct uwufs_inode root_inode;
	root_inode.access_flags = F_TYPE_REGULAR | 755;
	// NOTE: Maybe need to allocate a data block as well?

	root_inode_blk.inodes[2] = root_inode;

	memset(buf, 0, sizeof(buf));
	memcpy(buf, &root_inode_blk, sizeof(root_inode_blk));

	// TODO: write to actual block
}

/**
 * 	Formats the block device to uwufs format.
 *
 * 	`fd`: the opened block device
 * 	`total_blks`: total blocks in the opened block device
 * 	`reserved_space`: number of blocks reserved after superblock
 * 		and before the start of i-list
 * 	`ilist_percent`: percentage of total_blks used for i-list
 */
static int init_uwufs(int fd,
					  uwufs_blk_t total_blks,
					  uwufs_blk_t reserved_space,
					  float ilist_percent)
{
	// init i-list/i-nodes (nothing to be done)
	
	// init free list
	uwufs_blk_t first_free_blk = init_freelist(fd, total_blks, 
											reserved_space, ilist_percent);
	
	init_superblock(fd, total_blks, reserved_space, ilist_percent,
				  	first_free_blk);
#ifdef DEBUG
	printf("Initialized super block\n");
#endif
	
	// TODO: init root directory

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
		perror("Failed to access block device");
		return 1;
	}
	
	int ret = 0;

	// Get and check size of block device
	uwufs_blk_t blk_dev_size;
	ret = ioctl(fd, BLKGETSIZE64, &blk_dev_size);
	if (ret < 0) {
		perror("Not a block device or cannot determine size of device");
		close(fd);
		return 1;
	}
#ifdef DEBUG
	printf("Block device %s size: %ld\n", argv[1], blk_dev_size);
#endif

	// NOTE: Read user definable params later.
	// 	Specifing blk_dev_size to format can help with testing too
	ret = init_uwufs(fd, blk_dev_size/UWUFS_BLOCK_SIZE, UWUFS_RESERVED_SPACE,
				  	 UWUFS_ILIST_DEFAULT_SIZE);

	printf("Done formating device %s\n", argv[1]);
	close(fd);
	return ret;
}
