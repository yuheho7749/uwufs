/**
 * Header file for fuse syscall operations for uwufs
 *
 * Author: Joseph
 */

#ifndef SYSCALLS_H
#define SYSCALLS_H

#ifndef FUSE_USE_VERSION
#define FUSE_USE_VERSION 31
#endif

#include <fuse3/fuse.h>

/**
 * Set fuse connection parameters and configurations.
 *
 * NOTE: Effects may be limited in a virtual machine (virtual kernel,
 * 		memory, and devices)
 */
void* uwufs_init(struct fuse_conn_info *conn, struct fuse_config *cfg);

int uwufs_getattr(const char *path, struct stat *stbuf,
				  struct fuse_file_info *fi);

// NOTE: Might be uneeded because of uwufs_create
int uwufs_mknod(const char *path, mode_t mode, dev_t device);

int uwufs_mkdir(const char *path, mode_t mode);

/**
 * Decrements the hard link counter to a file. If the counter
 * 		becomes 0, delete the actual file.
 */
int uwufs_unlink(const char *path);

int uwufs_rmdir(const char *path);

int uwufs_open(const char *path, struct fuse_file_info *fi);

int uwufs_read(const char *path, char *buf, size_t size,
			   off_t offset, struct fuse_file_info *fi);

int uwufs_write(const char *path, const char *buf, size_t size,
				off_t offset, struct fuse_file_info *fi);

int uwufs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
				  off_t offset, struct fuse_file_info *fi,
				  enum fuse_readdir_flags flags);

int uwufs_create(const char *path, mode_t mode, struct fuse_file_info *fi);

int uwufs_utimens(const char *path, const struct timespec tv[2],
				  struct fuse_file_info *fi);

#endif
