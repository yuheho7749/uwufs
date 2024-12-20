/**
 * Formats block device in the uwufs format.
 *
 * Author: Joseph
 */


#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>

#ifdef __linux__
#include <linux/fs.h>
#endif

#include "uwufs.h"
#include "low_level_operations.h"
#include "file_operations.h"


/**
 * Creates a linked list structure for the freelist
 * 
 * NOTE:
 * Need to use a more efficient method of storing a freelist
 */
static uwufs_blk_t init_freelist(int fd,
								 uwufs_blk_t total_blks,
								 uwufs_blk_t freelist_start,
								 uwufs_blk_t freelist_size)
{
	uwufs_blk_t freelist_end = freelist_start + freelist_size;

#ifdef DEBUG
	printf("init_freelist: [start: %ld, end: %ld)\n",
		freelist_start, freelist_end);
#endif
	
	assert(freelist_end <= total_blks);

	// Connect the blocks in the free lists
	ssize_t bytes_written;
	uwufs_blk_t current_blk_num = freelist_start;
	uwufs_blk_t next_blk_num = current_blk_num + 1;
	struct uwufs_free_data_blk free_data_blk;
	while (current_blk_num < freelist_end - 1) {
		if ((current_blk_num - freelist_start) % (freelist_size/10) == 0) {
			printf("\tinit_freelist progress: %ld/%ld\n",
				current_blk_num - freelist_start, freelist_size);
		}

		// Connect to next block
		free_data_blk.next_free_blk = next_blk_num;
		
		// Write block to device
		bytes_written = write_blk(fd, &free_data_blk, current_blk_num);
		if (bytes_written != UWUFS_BLOCK_SIZE)
			goto error_exit;

		current_blk_num = next_blk_num;
		next_blk_num ++;
	}

	// Write 0 to last blk
	free_data_blk.next_free_blk = 0;

	bytes_written = write_blk(fd, &free_data_blk, current_blk_num);
	if (bytes_written != UWUFS_BLOCK_SIZE)
		goto error_exit;
	
	return freelist_start;

error_exit:
	perror("mkfs.uwu: failed writing freelist");
	close(fd);
	exit(1);
}

/**
 * Initializes the superblock. Assumes that `init_freelist` was already 
 * 		called and had initialized a freelist. The blk address of the
 * 		freelist head will be passed in `freelist_head`
 */
static void init_superblock(int fd,
							uwufs_blk_t total_blks, 
							uwufs_blk_t ilist_start,
							uwufs_blk_t ilist_total_size,
							uwufs_blk_t freelist_start,
							uwufs_blk_t freelist_total_size,
							uwufs_blk_t freelist_head)
{
	// Set super block values
	struct uwufs_super_blk super_blk;
	super_blk.total_blks = total_blks;
	super_blk.ilist_start = ilist_start;
	super_blk.ilist_total_size = ilist_total_size;
	super_blk.free_inodes_left = ilist_total_size * 
		(UWUFS_BLOCK_SIZE / sizeof(struct uwufs_inode)) - 3; // 0, 1, 2 are reserved/used
	super_blk.freelist_start = freelist_start;
	super_blk.freelist_total_size = freelist_total_size;
	super_blk.freelist_head = freelist_head;
	super_blk.free_blks_left = freelist_total_size - 1; // One block as buffer?

	// Write super block to device
	ssize_t bytes_written = write_blk(fd, &super_blk, 0);
	if (bytes_written != UWUFS_BLOCK_SIZE) {
		perror("mkfs.uwu: error writing superblock");
		close(fd);
		exit(1);
	}
}

/**
 * Set ilist blocks to all 0. This makes checking if an inode
 * 		is used very easy (Check the file mode if it is 0)
 */
static void init_inodes(int fd,
						uwufs_blk_t ilist_start,
						uwufs_blk_t ilist_total_size) {
	char zero_blk[UWUFS_BLOCK_SIZE];
	memset(zero_blk, 0, UWUFS_BLOCK_SIZE);

	uwufs_blk_t i;
	uwufs_blk_t ilist_end = ilist_start + ilist_total_size;
	ssize_t status;
	for (i = ilist_start; i < ilist_end; i ++) {
		if (i % (ilist_end/10) == 0) {
			printf("\tinit inodes blk progress: %ld/%ld\n",
				i - ilist_start, ilist_end - ilist_start);
		}
		status = write_blk(fd, zero_blk, i);
		if (status < 0) {
			perror("mkfs.uwu: error init inode blks");
			close(fd);
			exit(1);
		}
		
	}
}
	

/**
 * TEST:
 * Make inodes a linked list for easy allocation and deallocation of
 * 		inodes.
 */
static void init_inodes2(int fd,
						uwufs_blk_t ilist_start,
						uwufs_blk_t ilist_total_size) {
	uwufs_blk_t total_inodes = ilist_total_size * 
		(UWUFS_BLOCK_SIZE/sizeof(struct uwufs_inode));

	struct uwufs_inode free_inode;
	free_inode.file_mode = F_TYPE_FREE;

	// NOTE: Might want to make a inode linked list as well?

	uwufs_blk_t i;
	ssize_t status;
	for (i = 0; i < total_inodes; i ++) {
#ifdef DEBUG
		if (i % (total_inodes/20) == 0) {
			printf("\tinit inodes2 progress: %ld/%ld\n",
				i, total_inodes);
		}
#endif
		status = write_inode(fd, &free_inode, sizeof(free_inode), i);
		if (status < 0) {
			perror("mkfs.uwu: error init inodes2");
			close(fd);
			exit(1);
		}
	}
}

/**
 * Initialize root directory at inode UWUFS_ROOT_DIR_INODE
 */
static void init_root_directory(int fd)
{
	struct uwufs_inode root_inode;
	memset(&root_inode, 0, sizeof(root_inode));
	ssize_t status;
	time_t unix_time;
	
	// Add . and .. entry
	struct uwufs_directory_data_blk dir_blk;
	memset(&dir_blk, 0, sizeof(dir_blk));
	status = put_directory_file_entry(&dir_blk, ".", UWUFS_ROOT_DIR_INODE);
	if (status < 0)
		goto error_exit;
	status = put_directory_file_entry(&dir_blk, "..", UWUFS_ROOT_DIR_INODE);
	if (status < 0)
		goto error_exit;

	// Allocate a data block for . and ..
	uwufs_blk_t blk_num;
	status = malloc_blk(fd, &blk_num);
	if (status < 0 || blk_num <= 0)
		goto error_exit;

	// Write entries to actual data block
	status = write_blk(fd, &dir_blk, blk_num);
	if (status < 0)
		goto error_exit;

	// TODO: add other permissions, metadata, etc
	root_inode.file_mode = F_TYPE_DIRECTORY | 0755;
	root_inode.direct_blks[0] = blk_num;
	root_inode.file_size = UWUFS_BLOCK_SIZE;
	root_inode.file_links_count = 2; // Account for "." refer to itself
	root_inode.file_uid = 0;
	root_inode.file_gid = 0;
	unix_time = time(NULL);
	if (unix_time == -1) {
		root_inode.file_ctime = 0;
		root_inode.file_mtime = 0;
		root_inode.file_atime = 0;
	} else {
		root_inode.file_ctime = (uint64_t)unix_time;
		root_inode.file_mtime = (uint64_t)unix_time;
		root_inode.file_atime = (uint64_t)unix_time;
	}

	// Write to root inode
	status = write_inode(fd, &root_inode, sizeof(root_inode),
							  UWUFS_ROOT_DIR_INODE);
	if (status < 0)
		goto error_exit;

	return;

error_exit:
	perror("mkfs.uwu: error init root directory");
	close(fd);
	exit(1);
}

/**
 * Formats the block device to uwufs format.
 *
 * `fd`: the opened block device
 * `total_blks`: total blocks in the opened block device
 * `reserved_space`: number of blocks reserved after superblock
 * 		and before the start of i-list
 * `ilist_percent`: percentage of total_blks used for i-list
 */
static int init_uwufs(int fd,
					  uwufs_blk_t total_blks,
					  uwufs_blk_t reserved_space,
					  float ilist_percent)
{
	// Calculate where each region starts and stops
	uwufs_blk_t ilist_start = 1 + reserved_space;
	uwufs_blk_t ilist_size = (ilist_percent * total_blks);
	uwufs_blk_t freelist_start = ilist_start + ilist_size;
	uwufs_blk_t freelist_size = total_blks - 1 - reserved_space - ilist_size;

	printf("Initializing free list\n");
	uwufs_blk_t first_free_blk = init_freelist(fd, total_blks, freelist_start,
											freelist_size);

	printf("Initializing super block\n");
	init_superblock(fd, total_blks, ilist_start, ilist_size, freelist_start,
				 	freelist_size, first_free_blk);

	printf("Initializing inodes\n");
	init_inodes(fd, ilist_start, ilist_size);

	printf("Initializing root directory\n");
	init_root_directory(fd);

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

#ifdef __linux__
	// BLKGETSIZE64 assumes linux system
	ret = ioctl(fd, BLKGETSIZE64, &blk_dev_size);
#else
	perror("Unsupported operating system: not Linux");
	close(fd);
	return 1;
#endif
	if (ret < 0) {
		perror("Not a block device/partition or cannot determine its size");
		close(fd);
		return 1;
	}

	printf("Block device %s size : %ld bytes (%ld blocks)\n", argv[1],
		blk_dev_size, blk_dev_size/UWUFS_BLOCK_SIZE);

	printf("Formating device %s...\n", argv[1]);
	// NOTE: Read user definable params later.
	// 	Specifing blk_dev_size to format can help with testing too
#ifdef DEBUG
	// ret = init_uwufs(fd, 100, UWUFS_RESERVED_SPACE,
	// 			  	 0.5f);
	ret = init_uwufs(fd, blk_dev_size/UWUFS_BLOCK_SIZE, UWUFS_RESERVED_SPACE,
				  	 UWUFS_ILIST_DEFAULT_PERCENTAGE);
#else
	ret = init_uwufs(fd, blk_dev_size/UWUFS_BLOCK_SIZE, UWUFS_RESERVED_SPACE,
				  	 UWUFS_ILIST_DEFAULT_PERCENTAGE);
#endif

	printf("Done formating device %s\n", argv[1]);
	close(fd);
	return ret;
}
