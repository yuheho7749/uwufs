/**
 * 	Implement fuse syscall operations for uwufs
 *
 * 	Author: Joseph
 */

#define FUSE_USE_VERSION 31

#include <fuse3/fuse.h>

#include "low_level_operations.h"
#include "uwufs.h"
#include "syscalls.h"

#include <errno.h>
#include <string.h>
#include <stdio.h>

extern int device_fd;

void* uwufs_init(struct fuse_conn_info *conn,
				 struct fuse_config *cfg)
{
	(void) conn;
	// For testing fs journaling/recovery:
	// 		disables page caching in the kernel at the
	// 		cost of some performance
	cfg->direct_io = 1;
	cfg->use_ino = 1;
	return NULL;
}

// TODO:
int uwufs_getattr(const char *path,
				  struct stat *stbuf,
				  struct fuse_file_info *fi)
{
	(void) fi;

	// TODO: Use namei to search for the corresponding dir inode
	// TEMP: Hard coding for root directory only for now
	if (strcmp(path, "/") != 0)
		return -ENOENT; // TEMP:

	// uwufs_blk_t inode_num = namei(path);
	uwufs_blk_t inode_num = UWUFS_ROOT_DIR_INODE;

	memset(stbuf, 0, sizeof(struct stat));

	struct uwufs_inode inode;
	int status = read_inode(device_fd, &inode, inode_num);
	if (status < 0)
		return -ENOENT;

	// TODO: Fill in other file types and flags (not implemented yet)
	uwufs_aflags_t flags = inode.access_flags;
	switch (flags & F_TYPE_BITS) {
		case F_TYPE_DIRECTORY:
			stbuf->st_mode = S_IFDIR | (flags & F_PERM_BITS);
			stbuf->st_size = inode.file_size;
			stbuf->st_ino = inode_num;
			return 0;
		case F_TYPE_REGULAR:
			stbuf->st_mode = S_IFREG | (flags & F_PERM_BITS);
			stbuf->st_size = inode.file_size;
			stbuf->st_ino = inode_num;
			return 0;
		default:
			return -EINVAL;
	}
	return -ENOENT;
}

// TODO:
int uwufs_mkdir(const char *path,
				mode_t mode)
{
	// TEMP: don't care if it's not a dir
	if (!(mode | S_IFDIR))
		return -ENOTDIR;

	// TODO:
	return -ENOENT;
}

// TODO:
int uwufs_open(const char *path,
			   struct fuse_file_info *fi)
{
	return -ENOENT;
}

// TODO:
int uwufs_read(const char *path,
			   char *buf,
			   size_t size,
			   off_t offset,
			   struct fuse_file_info *fi)
{
	return -ENOENT;
}

// TODO:
int uwufs_write(const char *path,
				const char *buf,
				size_t size,
				off_t offset,
				struct fuse_file_info *fi)
{
	return -ENOENT;
}

static int uwufs_helper_readdir_blk(const struct uwufs_directory_data_blk blk,
									void *buf,
									fuse_fill_dir_t filler)
{
	int i;
	int status;
	int total_entires_per_blk = UWUFS_BLOCK_SIZE /
		sizeof(struct uwufs_directory_file_entry);
	for (i = 0; i < total_entires_per_blk; i ++) {
		if (blk.file_entries[i].inode_num <= 0) {
			continue; // assuming there are "holes"
		}
		status = filler(buf, blk.file_entries[i].file_name, NULL, 0,
			   FUSE_FILL_DIR_DEFAULTS);
		if (status == 1) {
			return status;
		}
	}
	return 0;
}

// NOTE: Choosing to operate in the first mode (see fuse documentation)
// 		1. Ignore offset and read the entire directory <<<
// 		2. Use offset but pass non-zero in 'filler'
int uwufs_readdir(const char *path,
				  void *buf,
				  fuse_fill_dir_t filler,
				  off_t offset,
				  struct fuse_file_info *fi,
				  enum fuse_readdir_flags flags)
{
	(void) offset;
	(void) fi;
	(void) flags;

	// TODO: Use namei to search for the corresponding dir inode
	// TEMP: Hard coding for root directory only for now
	if (strcmp(path, "/") != 0)
		return -ENOENT; // TEMP:

	// uwufs_blk_t inode_num = namei(path);
	uwufs_blk_t inode_num = UWUFS_ROOT_DIR_INODE;

	struct uwufs_inode inode;
	int status = read_inode(device_fd, &inode, inode_num);
	if (status < 0)
		return -ENOENT;

	uwufs_aflags_t aflags = inode.access_flags;
	if (F_TYPE_DIRECTORY != (aflags & F_TYPE_BITS))
		return -ENOTDIR;

	// TODO: Don't worry about permission bits yet (but still show it)


	// Read each direct data blk and add to filler
	int dir_blk_num = 0;
	int total_direct_blks = (inode.file_size + UWUFS_BLOCK_SIZE - 1) /
							 UWUFS_BLOCK_SIZE;
	struct uwufs_directory_data_blk dir_data_blk;

	// Loop through all direct block and fetch it one by one
	while (dir_blk_num < UWUFS_DIRECT_BLOCKS &&
		   dir_blk_num < total_direct_blks)
	{
		status = read_blk(device_fd, &dir_data_blk,
					inode.direct_blks[dir_blk_num]);
		if (status < 0)
			return -EIO;

		// At this point I have the dir data block
		status = uwufs_helper_readdir_blk(dir_data_blk, buf, filler);
		if (status == 1)
			// If buf is full, filler/helper will return 1
			// return error or just not include the rest?
			return 0;

		dir_blk_num ++;
	}
	// TODO: Dont' worry about indirects yet

	return 0;
}

// TODO:
int uwufs_create(const char *path,
				 mode_t mode,
				 struct fuse_file_info *fi)
{
	return -ENOENT;
}

// TODO:
int uwufs_utimens(const char *path,
				  const struct timespec tv[2],
				  struct fuse_file_info *fi)
{
	return -ENOENT;
}
