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

// TODO:
int uwufs_getattr(const char *path,
				  struct stat *stbuf,
				  struct fuse_file_info *fi)
{
	(void) fi;

	uwufs_blk_t inode_num;
	namei(device_fd, path, NULL, &inode_num);

	memset(stbuf, 0, sizeof(struct stat));

	struct uwufs_inode inode;
	int status = read_inode(device_fd, &inode, inode_num);
	if (status < 0)
		return -ENOENT;

	// TODO: Fill in other file types and flags (not implemented yet)
	uint16_t flags = inode.file_mode;
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


ssize_t split_path_parent_child(const char *path, char *parent_path, char *child_dir) {
    char path_copy[strlen(path)];
	strcpy(path_copy, path);
	char *last_dir = strrchr(path_copy, '/');
    
    if (last_dir != NULL) {
        size_t length = last_dir - path_copy;
        strncpy(parent_path, path_copy, length);
        parent_path[length] = '\0';

		size_t child_dir_length = strlen(last_dir+1); // start after '/'
		if (child_dir_length > UWUFS_FILE_NAME_SIZE) {
			strncpy(child_dir, last_dir+1, UWUFS_FILE_NAME_SIZE);
		}
		else {
			strncpy(child_dir, last_dir+1, child_dir_length);
		}
		child_dir[child_dir_length] = '\0';
		return 0;
    } else {
        // if no '/' is found, error
		// paths should all start from root (?)
#ifdef DEBUG
		printf("Error with split_path_parent_child() function\n");
#endif	
		return -1;
    }
}

// TODO: 
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
	// TEMP: don't care if it's not a dir
	if (!(mode | S_IFDIR))
		return -ENOTDIR;
	
	ssize_t status;

	// split path into parent + child parts
	char parent_path[strlen(path)];
	char child_dir[UWUFS_FILE_NAME_SIZE];
	status = split_path_parent_child(path, parent_path, child_dir);
	if (status < 0)
		return -ENOENT;

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

	// update the parent data blk 
	status = add_directory_file_entry(device_fd, parent_dir_inode_num,
						child_dir, child_dir_inode_num);

	// new child dir: populate . and .. entry
	struct uwufs_directory_data_blk new_dir_blk;
	memset(&new_dir_blk, 0, sizeof(new_dir_blk));
	status = put_directory_file_entry(&new_dir_blk, ".", child_dir_inode_num);
	RETURN_IF_ERROR(status);
	status = put_directory_file_entry(&new_dir_blk, "..", parent_dir_inode_num);
	RETURN_IF_ERROR(status);

	// allocate a new data blk
	uwufs_blk_t new_blk_num;
	status = malloc_blk(device_fd, &new_blk_num);
	if (status < 0 || new_blk_num <= 0)
		return status;

	// Write entries to actual data block
	status = write_blk(device_fd, &new_dir_blk, new_blk_num);
	RETURN_IF_ERROR(status);

	// TODO: add other permissions, metadata, etc
	struct uwufs_inode new_inode;
	memset(&new_inode, 0, UWUFS_INODE_DEFAULT_SIZE);
	new_inode.file_mode = F_TYPE_DIRECTORY | 0755;
	new_inode.direct_blks[0] = new_blk_num;
	new_inode.file_size = UWUFS_BLOCK_SIZE;

	// write new dir inode
	status = write_inode(device_fd, &new_inode, sizeof(new_inode),
						 child_dir_inode_num);
	RETURN_IF_ERROR(status);
	
	return 0;
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

	uwufs_blk_t inode_num;
	namei(device_fd, path, NULL, &inode_num);

	struct uwufs_inode inode;
	int status = read_inode(device_fd, &inode, inode_num);
	if (status < 0)
		return -ENOENT;

	uint16_t aflags = inode.file_mode;
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
