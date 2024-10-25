/**
 * 	Author: Joseph
 */

#include "low_level_operations.h"

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

ssize_t read_blk(int fd, char* buf, uwufs_blk_t blk_num)
{
	// TODO:
	return -1;
}

ssize_t write_blk(int fd, char* buf, size_t size, uwufs_blk_t blk_num)
{
	int status = lseek(fd, blk_num, SEEK_SET);
	if (status < 0) {
#ifdef DEBUG
		perror("write_blk: cannot lseek to blk");
#endif
		return -1;
	}
#ifdef DEBUG
	assert(sizeof(buf) == UWUFS_BLOCK_SIZE);
#endif
	ssize_t bytes_written = write(fd, buf, UWUFS_BLOCK_SIZE);
	if (bytes_written != UWUFS_BLOCK_SIZE) {
#ifdef DEBUG
		perror("write_blk: error writing to blk");
#endif
		return bytes_written;
	}
	return bytes_written;
}
