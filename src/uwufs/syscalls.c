/**
 * Implement fuse syscall operations for uwufs
 *
 * Authors: Joseph, Kay
 */

#define FUSE_USE_VERSION 31

#include <fuse3/fuse.h>
#include "file_operations.h"
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

int uwufs_getattr(const char *path,
				  struct stat *stbuf,
				  struct fuse_file_info *fi)
{
	(void) fi;

	// TODO: Check file permissions

	uwufs_blk_t inode_num;
	ssize_t status = namei(device_fd, path, NULL, &inode_num);
	if (status < 0)
		return -ENOENT;

	memset(stbuf, 0, sizeof(struct stat));

	struct uwufs_inode inode;
	status = read_inode(device_fd, &inode, inode_num);
	if (status < 0)
		return -ENOENT;

	uint16_t f_mode = inode.file_mode;
	switch (f_mode & F_TYPE_BITS) {
		case F_TYPE_DIRECTORY:
			stbuf->st_mode = S_IFDIR | (f_mode & F_PERM_BITS);
			break;
		case F_TYPE_REGULAR:
			stbuf->st_mode = S_IFREG | (f_mode & F_PERM_BITS);
			break;
		default:
			return -EINVAL;
	}
	// TODO: Fill in other file types and flags (not implemented yet)
	stbuf->st_ino = inode_num;
	stbuf->st_size = inode.file_size;
	stbuf->st_blksize = UWUFS_BLOCK_SIZE;
	stbuf->st_blocks = ((inode.file_size + UWUFS_BLOCK_SIZE - 1)
						/ UWUFS_BLOCK_SIZE) * 8;
	stbuf->st_nlink = inode.file_links_count;
	stbuf->st_uid = inode.file_uid;
	stbuf->st_gid = inode.file_gid;
	stbuf->st_ctime = inode.file_ctime;
	stbuf->st_mtime = inode.file_mtime;
	stbuf->st_atime = inode.file_atime;
	return 0;
}

// NOTE: Might be uneeded because of uwufs_create
int uwufs_mknod(const char *path, mode_t mode, dev_t device)
{
	return -ENOENT;
}

// 1) namei() with the path up to the directory to be created
// return if namei errors or the lead-up path isn't found
// 2) find_free_inode() to get the inode to use 
// 3) update the parent dir data blk with a new entry
// 3) malloc & write the new dir data block
// 	- an entry for '.' and '..'
// 4) write the new dir inode
int uwufs_mkdir(const char *path,
				mode_t mode)
{
	printf("mkdir %s\n", path);
	ssize_t status;
	time_t unix_time;
	// get the uid etc of the user
	struct fuse_context *fuse_ctx = fuse_get_context();

	// FIX: Here to make sure the append_dblk in add_directory_entry
	// always have enough data blocks (remove it after the bug is fixed)
	struct uwufs_super_blk super_blk;
	status = read_blk(device_fd, &super_blk, 0);
	if (status < 0)
		return status;
	if (super_blk.free_blks_left <= 7) {
		return -ENOSPC;
	}

	// printf("original path %s", path);
	// split path into parent + child parts
	char parent_path[strlen(path)+1];
	char child_dir[UWUFS_FILE_NAME_SIZE];
	status = split_path_parent_child(path, parent_path, child_dir);
	if (status < 0)
		return status;
	// printf("parent_path %s", parent_path);
	// printf("child_dir %s", child_dir);

	// read root inode for namei
	struct uwufs_inode root_inode;
	status = read_inode(device_fd, &root_inode, UWUFS_ROOT_DIR_INODE);
	RETURN_IF_ERROR(status);

	// find the parent path inode
	uwufs_blk_t parent_dir_inode_num;
	status = namei(device_fd, parent_path, &root_inode, &parent_dir_inode_num);
	if (status < 0)
		return -ENOENT;

	// find free inode for new child dir
	uwufs_blk_t child_dir_inode_num;
	status = find_free_inode(device_fd, &child_dir_inode_num);
	RETURN_IF_ERROR(status);

	// allocate a new data blk
	uwufs_blk_t new_blk_num;
	status = malloc_blk(device_fd, &new_blk_num);
	if (status < 0 || new_blk_num <= 0)
		return status;

	// update the parent data blk 
	status = add_directory_file_entry(device_fd, parent_dir_inode_num,
						child_dir, child_dir_inode_num, 1);
	if (status < 0) {
		free_blk(device_fd, new_blk_num);
		return status;
	}
	// RETURN_IF_ERROR(status);

	// new child dir: populate . and .. entry
	struct uwufs_directory_data_blk new_dir_blk;
	memset(&new_dir_blk, 0, sizeof(new_dir_blk));
	status = put_directory_file_entry(&new_dir_blk, ".", child_dir_inode_num);
	if (status < 0) {
		free_blk(device_fd, new_blk_num);
		return status;
	}
	// RETURN_IF_ERROR(status);
	status = put_directory_file_entry(&new_dir_blk, "..", parent_dir_inode_num);
	if (status < 0) {
		free_blk(device_fd, new_blk_num);
		return status;
	}
	// RETURN_IF_ERROR(status);

	// Write entries to actual data block
	status = write_blk(device_fd, &new_dir_blk, new_blk_num);
	// RETURN_IF_ERROR(status);
	if (status < 0) {
		free_blk(device_fd, new_blk_num);
		return status;
	}

	// TODO: add other permissions, metadata, etc
	struct uwufs_inode new_inode;
	memset(&new_inode, 0, sizeof(new_inode));
	new_inode.file_mode = F_TYPE_DIRECTORY | (F_PERM_BITS & mode);
	new_inode.direct_blks[0] = new_blk_num;
	new_inode.file_size = UWUFS_BLOCK_SIZE;
	new_inode.file_links_count = 2; // includes "." refer to itself
	new_inode.file_uid = fuse_ctx->uid;
	new_inode.file_gid = fuse_ctx->gid;
	unix_time = time(NULL);
	if (unix_time == -1)
		unix_time = 0;
	new_inode.file_ctime = (uint64_t)unix_time;
	new_inode.file_mtime = (uint64_t)unix_time;
	new_inode.file_atime = (uint64_t)unix_time;

	// write new dir inode
	status = write_inode(device_fd, &new_inode, sizeof(new_inode),
						 child_dir_inode_num);
	// RETURN_IF_ERROR(status);
	if (status < 0) {
		free_blk(device_fd, new_blk_num);
		return status;
	}
	
	return 0;
}

int uwufs_unlink(const char *path)
{
	uwufs_blk_t inode_num;
	struct uwufs_inode inode;
	ssize_t status = namei(device_fd, path, NULL, &inode_num);
	if (status < 0)
		return -ENOENT;

	status = read_inode(device_fd, &inode, inode_num);
	if (status < 0)
		return status;

	// TODO: Check file permissions using fuse_context

	switch (inode.file_mode & F_TYPE_BITS) {
		case F_TYPE_REGULAR:
		// case F_TYPE_DIRECTORY: // should be handled by rmdir
			status = unlink_file(device_fd, path, &inode, inode_num, 0);
			if (status < 0) return status;

			// check if link count is 0
			if (inode.file_links_count == 0) {
				// free the data blk and inode
				status = remove_file(device_fd, &inode, inode_num);
				if (status < 0) return status;
			}

			// write back to inode
			status = write_inode(device_fd, &inode, sizeof(inode), inode_num);
			if (status < 0) return status;

			return 0;
		// TODO: other file types (Ex: symlinks don't have data blks)
		default:
#ifdef DEBUG
			printf("uwufs_unlink: unknown file type %d\n",
		  inode.file_mode & F_TYPE_BITS);
#endif
			return -EINVAL;
	}
	return -EIO;
}


int uwufs_rmdir(const char *path)
{	
	// split parent & child parts
	ssize_t status;
	char parent_path[strlen(path)+1];
	char child_path[UWUFS_FILE_NAME_SIZE];
	status = split_path_parent_child(path, parent_path, child_path);
	RETURN_IF_ERROR(status);

	if (strcmp(child_path, ".") == 0)
		return -EINVAL;
	if (strcmp(child_path, "..") == 0)
		return -ENOTEMPTY;

	// checking if the user is attempting to remove the root directory
	if (strcmp(parent_path, "") == 0 &&
		strcmp(child_path, "") == 0 )  {
			return -EPERM; // not sure if better error code to use here
	}

	// get child inode
	uwufs_blk_t child_dir_inode_num;
	status = namei(device_fd, path, NULL, &child_dir_inode_num);
	RETURN_IF_ERROR(status);

	// read child inode 
	struct uwufs_inode child_dir_inode;
	status = read_inode(device_fd, &child_dir_inode, child_dir_inode_num);
	RETURN_IF_ERROR(status);

	if ((child_dir_inode.file_mode & F_TYPE_BITS) != F_TYPE_DIRECTORY)
		return -ENOTDIR;

	// TODO: check permissions to see if user allowed to rmdir

	// check if dir empty (assume entries are semi packed - see is_directory_empty)
	if (!is_directory_empty(device_fd, &child_dir_inode))
		return -ENOTEMPTY;

	// check only 2 links as well
	// NOTE: this shouldn't happen, but handle in case 
	if (child_dir_inode.file_links_count != 2) {
#ifdef DEBUG
		printf("The directory has %d links but also has exactly 2 entries\n",
				 child_dir_inode.file_links_count);
#endif			
		return -ENOTEMPTY;
	}

	// remove the entry for the child dir from the parent dir blks
	status = unlink_file(device_fd, path, &child_dir_inode, 
				         child_dir_inode_num, -1);
	if (status < 0) return -EIO;

	// remove child dir (sets inode to FREE & clears data blks)
	status = remove_file(device_fd, &child_dir_inode, child_dir_inode_num);
	if (status < 0) return -EIO;
	status = write_inode(device_fd, &child_dir_inode, 
						 sizeof(struct uwufs_inode), child_dir_inode_num);
	if (status < 0) return -EIO;

	return 0;
}

// TODO:
int uwufs_open(const char *path,
			   struct fuse_file_info *fi)
{
	uwufs_blk_t inode_num;
	ssize_t status = namei(device_fd, path, NULL, &inode_num);
	if (status < 0)
		return -ENOENT;
	
	return 0;
	//return -ENOENT;
}

// TODO:
int uwufs_read(const char *path,
			   char *buf,
			   size_t size,
			   off_t offset,
			   struct fuse_file_info *fi)
{
	(void) fi;
	printf("in uwufs_read\n");
	
	uwufs_blk_t inode_num;
	struct uwufs_inode inode;
	ssize_t status = namei(device_fd, path, NULL, &inode_num);
	if (status < 0)
		return -ENOENT;

	status = read_inode(device_fd, &inode, inode_num);
	RETURN_IF_ERROR(status);

	// TODO: Check file permissions using fuse_context
	switch (inode.file_mode & F_TYPE_BITS) {
		case F_TYPE_REGULAR:
			status = read_file(device_fd, buf, size, offset, &inode);
			if (status < 0)
				return -EIO;

			return status;
		case F_TYPE_DIRECTORY:
			return -EISDIR;
		
		// TODO: other file types (Ex: symlinks don't have data blks)
		default:
#ifdef DEBUG
			printf("uwufs_unlink: unknown file type %d\n",
		  inode.file_mode & F_TYPE_BITS);
#endif
			return -EINVAL;
	}

	return -ENOENT;
}

// TODO:
int uwufs_write(const char *path,
				const char *buf,
				size_t size,
				off_t offset,
				struct fuse_file_info *fi)
{
	(void) fi;
	printf("in uwufs_write\n");
	
	uwufs_blk_t inode_num;
	struct uwufs_inode inode;
	ssize_t status = namei(device_fd, path, NULL, &inode_num);
	if (status < 0)
		return -ENOENT;

	status = read_inode(device_fd, &inode, inode_num);
	RETURN_IF_ERROR(status);

	// TODO: Check file permissions using fuse_context
	switch (inode.file_mode & F_TYPE_BITS) {
		case F_TYPE_REGULAR:
		// case F_TYPE_DIRECTORY: // should be handled by readdir
			status = write_file(device_fd, buf, size, offset, 
				 			    &inode, inode_num);
			if (status < 0)
				return -EIO;

			return status;
		// TODO: other file types (Ex: symlinks don't have data blks)
		default:
#ifdef DEBUG
			printf("uwufs_unlink: unknown file type %d\n",
		  inode.file_mode & F_TYPE_BITS);
#endif
			return -EINVAL;
	}

	return -ENOENT;
}

static int __uwufs_helper_readdir_blk(const struct uwufs_directory_data_blk blk,
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

	uwufs_blk_t inode_num;
	ssize_t status = namei(device_fd, path, NULL, &inode_num);
	if (status < 0)
		return -ENOENT;

	struct uwufs_inode inode;
	status = read_inode(device_fd, &inode, inode_num);
	if (status < 0)
		return -ENOENT;

	uint16_t f_mode = inode.file_mode;
	if (F_TYPE_DIRECTORY != (f_mode & F_TYPE_BITS))
		return -ENOTDIR;

	// TODO: Don't worry about permission bits yet (but still show it)

	dblk_itr_t dblk_itr = create_dblk_itr(&inode, device_fd, 0);
	// Read each data blk and add to filler
	uwufs_blk_t dir_data_blk_num;
	int total_blks = (inode.file_size + UWUFS_BLOCK_SIZE - 1) /
							 UWUFS_BLOCK_SIZE;
	struct uwufs_directory_data_blk dir_data_blk;
	int i;
	for (i = 0; i < total_blks; i++) {
		dir_data_blk_num = dblk_itr_next(dblk_itr);
		if (dir_data_blk_num == 0) { // Shouldn't happen
			destroy_dblk_itr(dblk_itr);
			return -EIO;
		}

		status = read_blk(device_fd, &dir_data_blk, dir_data_blk_num);
		if (status < 0) {
			destroy_dblk_itr(dblk_itr);
			return -EIO;
		}

		status = __uwufs_helper_readdir_blk(dir_data_blk, buf, filler);
		if (status == 1) {
			// If buf is full, filler/helper will return 1
			// return error or just not include the rest?
			destroy_dblk_itr(dblk_itr);
			return 0;
		}
	}
	destroy_dblk_itr(dblk_itr);
	return 0;
}

// NOTE: Might want to move to file_operations if not using fuse_file_info
int __create_regular_file(const char *path,
						  mode_t mode,
						  struct fuse_file_info *fi)
{
	(void) fi; // TEMP: Don't care for now

	ssize_t file_exists_status;

	char parent_path[strlen(path)+1];
	char child_path[UWUFS_FILE_NAME_SIZE];

	uwufs_blk_t parent_dir_inode_num;

	uwufs_blk_t child_file_inode_num;
	struct uwufs_inode child_file_inode;

	struct fuse_context *fuse_ctx = fuse_get_context();
	time_t unix_time;
	unix_time = time(NULL);
	if (unix_time == -1)
		unix_time = 0;

	ssize_t status;

	// FIX: Here to make sure the append_dblk in add_directory_entry
	// always have enough data blocks (remove it after the bug is fixed)
	struct uwufs_super_blk super_blk;
	status = read_blk(device_fd, &super_blk, 0);
	if (status < 0)
		return status;
	if (super_blk.free_blks_left <= 5) {
		return -ENOSPC;
	}

	status = split_path_parent_child(path, parent_path, child_path);
	if (status < 0)
		return status;

	file_exists_status = namei(device_fd, path, NULL, &child_file_inode_num);
	if (file_exists_status == -ENOENT) { // Create new file here
#ifdef DEBUG
		printf("__create_regular_file: creating new file\n");
#endif
		// Find parent inode
		status = namei(device_fd, parent_path, NULL, &parent_dir_inode_num);
		if (status < 0)
			return -ENOENT;

		// Get new empty inode
		status = find_free_inode(device_fd, &child_file_inode_num);
		RETURN_IF_ERROR(status);

		// Add child file entry to parent dir
		status = add_directory_file_entry(device_fd, parent_dir_inode_num,
						   child_path, child_file_inode_num, 0);
		RETURN_IF_ERROR(status);

		// Init child file inode
		memset(&child_file_inode, 0, sizeof(struct uwufs_inode));
		// TODO: Other file perms
		child_file_inode.file_mode = F_TYPE_REGULAR | (mode & F_PERM_BITS);
		child_file_inode.file_size = 0;
		child_file_inode.file_links_count = 1;
		child_file_inode.file_uid = fuse_ctx->uid;
		child_file_inode.file_gid = fuse_ctx->gid;
		child_file_inode.file_ctime = (uint64_t)unix_time;
		child_file_inode.file_mtime = (uint64_t)unix_time;

		goto success_ret;
	} else if (file_exists_status < 0) {
		return file_exists_status;
	}
#ifdef DEBUG
	printf("create_regular_file: file already exists");
#endif
success_ret:
	child_file_inode.file_atime = (uint64_t)unix_time;
	status = write_inode(device_fd, &child_file_inode,
				   sizeof(child_file_inode), child_file_inode_num);
	if (status < 0)
		return -EIO;

	return 0;
}

int uwufs_create(const char *path,
				 mode_t mode,
				 struct fuse_file_info *fi)
{
#ifdef DEBUG
	printf("uwufs_create: %s\n", path);
#endif
	if (S_ISREG(mode))
		return  __create_regular_file(path, mode, fi);
	return -EINVAL;
}

int uwufs_utimens(const char *path,
				  const struct timespec tv[2],
				  struct fuse_file_info *fi)
{
	(void) fi;
	ssize_t status;
	uwufs_blk_t inode_num;
	struct uwufs_inode inode;
	time_t unix_time;
	unix_time = time(NULL);
	if (unix_time == -1)
		unix_time = 0;

	status = namei(device_fd, path, NULL, &inode_num);
	if (status < 0)
		return status;

	status = read_inode(device_fd, &inode, inode_num);
	if (status < 0)
		return status;

	if (tv[0].tv_nsec == UTIME_NOW) {
		inode.file_atime = (uint64_t)unix_time;
	} else if (tv[0].tv_nsec == UTIME_OMIT) {
		// Do nothing
	} else {
		inode.file_atime = (uint64_t)tv[0].tv_sec;
	}
	if (tv[1].tv_nsec == UTIME_NOW) {
		inode.file_atime = (uint64_t)unix_time;
	} else if (tv[1].tv_nsec == UTIME_OMIT) {
		// Do nothing
	} else {
		inode.file_atime = (uint64_t)tv[1].tv_sec;
	}

	status = write_inode(device_fd, &inode, sizeof(inode), inode_num);
	if (status < 0)
		return status;

	return 0;
}
