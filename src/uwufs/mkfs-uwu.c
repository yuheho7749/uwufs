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

#include "uwufs.h"

char buf[UWUFS_BLOCK_SIZE];

// TODO: Start with simple linked list
static uwufs_blknum_t init_freelist(int fd) 
{
	
	return -1;
}

static void init_superblock(int fd, uwufs_blknum_t free_blk) 
{
	memset(buf, 0, sizeof(buf));
	int status = lseek(fd, 0, SEEK_SET);
	if (status != 0) {
		perror("mkfs.uwu: cannot lseek to beginning to init superblock");
		close(fd);
		exit(1);
	}
	// TODO: superblock data
	

	ssize_t bytes_written = write(fd, buf, UWUFS_BLOCK_SIZE);
	if (bytes_written != UWUFS_BLOCK_SIZE) {
		perror("mkfs.uwu: error writing superblock");
		close(fd);
		exit(1);
	}
}

// TODO:
static int init_uwufs(int fd)
{
	// init i-list/i-nodes (nothing to be done)
	
	// init free list
	uwufs_blknum_t free_blk = init_freelist(fd);
	
	init_superblock(fd, free_blk);
	
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
		perror("Failed to access block device");
		return 1;
	}

	int ret = init_uwufs(fd);

	close(fd);
	return ret;
}
