/**
 * Launches the fuse daemon and mounts the block device to a directory
 *
 * Author: Joseph
 */

#define FUSE_USE_VERSION 31

#include <fuse3/fuse.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>

#ifdef __linux__
#include <linux/fs.h>
#endif

#include "uwufs.h"
#include "syscalls.h"

int device_fd;

static const struct fuse_operations uwufs_oper = {
	.getattr	= uwufs_getattr,
	.mkdir		= uwufs_mkdir,
	.unlink		= uwufs_unlink,
	.rmdir		= uwufs_rmdir,
	.chmod		= uwufs_chmod,
	.chown		= uwufs_chown,
	.open		= uwufs_open,
	.read		= uwufs_read,
	.write		= uwufs_write,
	.release	= uwufs_release,
	.readdir	= uwufs_readdir,
	.init       = uwufs_init,
	.create 	= uwufs_create,
	.utimens 	= uwufs_utimens,
};

int main(int argc, char *argv[]) {
	if (argc < 3) {
		printf("Usage: %s [device] [mountpoint] [optional: flags]\n", argv[0]);
		return 1;
	}

	struct stat stbuf;

	if (stat(argv[2], &stbuf) == -1) {
		perror("Failed to access mountpoint");	
		return 1;
	}

	if (!S_ISDIR(stbuf.st_mode)) {
		perror("Mountpoint must be a directory");
		return 1;
	}

	device_fd = open(argv[1], O_RDWR);
	if (device_fd < 0) {
		perror("Failed to access block device");
		return 1;
	}

	int ret = 0;
	uwufs_blk_t blk_dev_size;

#ifdef __linux__
	// BLKGETSIZE64 assumes linux system
	ret = ioctl(device_fd, BLKGETSIZE64, &blk_dev_size);
#else
	perror("Unsupported operating system: not Linux");
	close(device_fd);
	return 1;
#endif
	if (ret < 0) {
		perror("Not a block device/partition or cannot determine its size");
		close(device_fd);
		return 1;
	}

	// NOTE: LATER - Check integrity of block device/partition
#ifdef DEBUG
	printf("Skip checking integrity of block device/partition...\n");
#endif

	printf("Mounting '%s' to '%s'...\n", argv[1], argv[2]);

	argv[1] = argv[0];
	ret = fuse_main(argc - 1, &argv[1], &uwufs_oper, NULL);
	close(device_fd);
	return ret;
}
