/**
 * 	Implement syscall operations for uwufs
 *
 * 	Author: Joseph
 */

#include "syscalls.h"

#include <errno.h>

void* uwufs_init(struct fuse_conn_info *conn,
				 struct fuse_config *cfg)
{
	(void) conn;
	// For testing fs journaling/recovery:
	// 		disables page caching in the kernel at the
	// 		cost of some performance
	cfg->direct_io = 1;
	return NULL;
}

// TODO:
int uwufs_getattr(const char *path,
				  struct stat *stbuf,
				  struct fuse_file_info *fi)
{
	return -ENOENT;
}

// TODO:
int uwufs_mkdir(const char *path,
				mode_t mode)
{
	// TEMP: don't care if it's not a dir
	if (!(mode | S_IFDIR)) {
		return -ENOTDIR;
	}

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

// TODO:
int uwufs_readdir(const char *path,
				  void *buf,
				  fuse_fill_dir_t filler,
				  off_t offset,
				  struct fuse_file_info *fi,
				  enum fuse_readdir_flags flags)
{
	return -ENOENT;
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
