/**
 *	Formats block device in the uwufs format.
 *
 *	Author: Joseph
 */


#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <linux/fs.h>

#include "uwufs.h"
#include "low_level_operations.h"

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
	uwufs_blk_t freelist_end = total_blks;

#ifdef DEBUG
	printf("init_freelist: (start: %ld, end: %ld)\n", freelist_start, freelist_end);
#endif

	// Connect the blocks in the free lists
	uwufs_blk_t current_blk_num = freelist_start;
	uwufs_blk_t next_blk_num = current_blk_num + 1;
	struct uwufs_free_data_blk free_data_blk;
	while (next_blk_num < freelist_end) {
#ifdef DEBUG
		if (current_blk_num % (freelist_end/100) == 0) {
			printf("\tinit_freelist progress: %ld/%ld\n",
				current_blk_num, freelist_end);
		}
#endif

		// Connect to next block
		free_data_blk.next_free_blk = next_blk_num;
		
		// Write block to device
		ssize_t bytes_written = write_blk(fd, &free_data_blk, UWUFS_BLOCK_SIZE, current_blk_num);
		if (bytes_written != UWUFS_BLOCK_SIZE) {
			perror("mkfs.uwu: failed writing freelist");
			close(fd);
			exit(1);
		}

		current_blk_num = next_blk_num;
		next_blk_num ++;
	}

	// Write 0 to last blk
	free_data_blk.next_free_blk = 0;

	ssize_t bytes_written = write_blk(fd, &free_data_blk, UWUFS_BLOCK_SIZE,
								      current_blk_num);
	if (bytes_written != UWUFS_BLOCK_SIZE) {
		perror("mkfs.uwu: failed writing freelist");
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
	// Set super block values
	struct uwufs_super_blk super_blk;
	super_blk.total_blks = total_blks;
	super_blk.freelist_head = first_free_blk;
	super_blk.ilist_start = 1 + reserved_space; // after super + reserved
	super_blk.ilist_size = ilist_percent * super_blk.total_blks;	

	// Write super block to device
	ssize_t bytes_written = write_blk(fd, &super_blk, UWUFS_BLOCK_SIZE, 0);
	if (bytes_written != UWUFS_BLOCK_SIZE) {
		perror("mkfs.uwu: error writing superblock");
		close(fd);
		exit(1);
	}
}

static void init_root_directory(int fd)
{
	struct uwufs_inode root_inode;
	root_inode.access_flags = F_TYPE_REGULAR | 755;
	// NOTE: Maybe need to allocate a data block as well?

	// write to actual block
	ssize_t status = write_inode(fd, &root_inode, sizeof(struct uwufs_inode), 2);
	if (status < 0) {
		perror("mkfs.uwu: error init root directory");
		close(fd);
		exit(1);
	}
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
	
#ifdef DEBUG
	printf("Initializing free list\n");
#endif
	uwufs_blk_t first_free_blk = init_freelist(fd, total_blks, 
											reserved_space, ilist_percent);
#ifdef DEBUG
	printf("Initialized free list\n");
#endif
	
#ifdef DEBUG
	printf("Initializing superblock\n");
#endif
	init_superblock(fd, total_blks, reserved_space, ilist_percent,
				  	first_free_blk);
#ifdef DEBUG
	printf("Initialized super block\n");
#endif

#ifdef DEBUG
	printf("Initializing root directory\n");
#endif
	init_root_directory(fd);
#ifdef DEBUG
	printf("Initialized root directory\n");
#endif

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
