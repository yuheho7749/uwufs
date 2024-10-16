/**
 * 	Phase 1 of cs270 final project.
 *
 * 	Description: Send "spam" mail to professor when writing to any file
 * 		in the mounted fuse file system "/cs270"
 *
 * 	Author: Joseph
 * 	Collaborators: Kay, Jason, Kong
 */

#define FUSE_USE_VERSION 31

#include <fuse3/fuse.h>
#include <fuse3/fuse_lowlevel.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define MAX_FILES 3 // TEST:
#define SPAM_MAIL 0


// TEMP: Phase 1: Stores the files in memory 
static struct fake_filesystem {
	char *file_names[MAX_FILES];
	char *file_contents[MAX_FILES];
	int no_files;
} fake_filesystem;

// TEMP: Helper to search for files in our fake filesystem
static int search_file(const char *path) {

	int i;
	char *f_name;
	for (i = 0; i < fake_filesystem.no_files; i++) {
		f_name = fake_filesystem.file_names[i];
		if (strcmp(path, f_name) == 0) {
			return i;
		}
	}
	return -1;
}


static void* spam_mail_init(struct fuse_conn_info *conn,
							struct fuse_config *cfg) {
	(void) conn;
	cfg->direct_io = 1; 	// disables page caching in the kernel
	return NULL;
}

/**
 * 	Gets meta data from file. 
 * 	Example: the file permissions, uid, gid, time accessed, etc
 *
 * 	In phase 1, we are hard coding the permissions in stbuf. In phase 2, we 
 * 	will read the raw bytes from our fs and set the correct output values
 * 	in stbuf
 */
static int spam_mail_getattr(const char *path, struct stat *stbuf, 
							 struct fuse_file_info *fi) {
	(void) fi;	

	memset(stbuf, 0, sizeof(struct stat));
	if (strcmp(path, "/") == 0) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
		// A directory has at least 2 hard links because "." 
		// inside itself also refers to itself
		return 0;
	} 

	int file_id;
	file_id = search_file(path);
	if (file_id >= 0) { 
		// TEMP: Hard coding the file information (except for the contents)
		stbuf->st_mode = S_IFREG | 0644;
		stbuf->st_nlink = 1;

		stbuf->st_size = strlen(fake_filesystem.file_contents[file_id]);
		// assuming block size of 4k bytes (4096 bytes)
		stbuf->st_blocks = (stbuf->st_size + 4095) / 4096;

		stbuf->st_uid = getuid();
		stbuf->st_gid = getgid();

		stbuf->st_atime = time(NULL);
		stbuf->st_mtime = stbuf->st_atime;
		stbuf->st_ctime = stbuf->st_atime;
		return 0;
	}

	return -ENOENT;
}

/**
 *	Similar to getattr, but for directories
 */
static int spam_mail_readdir(const char *path, void *buf, 
							 fuse_fill_dir_t filler, off_t offset, 
							 struct fuse_file_info *fi, 
							 enum fuse_readdir_flags flags) {
	(void) offset;
	(void) fi;
	(void) flags;


	if (strcmp(path, "/") != 0) {
		return -ENOENT;
	}
	
	filler(buf, ".", NULL, 0, FUSE_FILL_DIR_DEFAULTS);
	filler(buf, "..", NULL, 0, FUSE_FILL_DIR_DEFAULTS);

	int i;
	for (i = 0; i < fake_filesystem.no_files; i++) {
		filler(buf, fake_filesystem.file_names[i] + 1, NULL, 0, FUSE_FILL_DIR_DEFAULTS);
	}

	return 0;
}

/**
 *	Syscall to create a new file
 *
 *	TODO:
 *	Phase 1 Requirements: We can fake it by creating the file info in memory, 
 *		but that is not good for phase 2
 *	
 *	Phase 2 Requirements: We need to create this file in storage
 */
static int spam_mail_create(const char *path, mode_t mode, 
							struct fuse_file_info *fi) {
	(void) fi;

	// TEST:
	printf("create file path: %s\n", path);

	// // NOTE: Storing it in our in memory fake_filesystem
	if (fake_filesystem.no_files >= MAX_FILES) {
		return -ENOSPC;
	}

	// TEMP: Ignore the file create mode for now
	int file_id;
	file_id = fake_filesystem.no_files;
	fake_filesystem.file_names[file_id] = strdup(path);
	fake_filesystem.file_contents[file_id] = strdup("");
	fake_filesystem.no_files ++;

	return 0;
}

static int spam_mail_open(const char *path, struct fuse_file_info *fi) {
	int file_id;
	file_id = search_file(path);  // TEMP: Using fake filesystem
	if (file_id < 0) {
		return -ENOENT;
	}

	// TODO:
	// if ((fi->flags & O_ACCMODE) != O_RDONLY)
	// 	return -EACCES;

	return 0;
}

static int spam_mail_read(const char *path, char *buf, size_t size, 
						  off_t offset, struct fuse_file_info *fi) {
	(void) fi;

	int file_id;
	file_id = search_file(path);  // TEMP: Using fake filesystem
	if (file_id < 0) {
		return -ENOENT;
	}

	int len;
	len = strlen(fake_filesystem.file_contents[file_id]);
	if (offset < len) {
		if (offset + size > len) {
			size = len - offset;
		}
		memcpy(buf, fake_filesystem.file_contents[file_id] + offset, size);
	} else {
		size = 0;
	}

	return size;
}

static int spam_mail_write(const char *path, const char *buf, size_t size, 
						   off_t offset, struct fuse_file_info *fi) {
	
	(void) path;
	(void) fi; // This is important if this is 

	int file_id;
	file_id = search_file(path);  // TEMP: Using fake filesystem
	if (file_id < 0) {
		return -ENOENT;
	}

	memcpy(fake_filesystem.file_contents[file_id] + offset, buf, size);

	if (SPAM_MAIL) {
		// ISSUE: SECURITY ISSUE!!!
		// Taking in raw user input as command is bad!!!
		char command[10000];

		// TODO: Change to professor's email: rich@cs.ucsb.edu
		snprintf(command, 10000, "echo \"%s\" | mail -s \"Fuse Mail!\" joseph_ng@ucsb.edu", buf);

		printf("Attempting to send mail\n");
		system(command);
		printf("Sent mail\n");
	}

	return size;
}

static const struct fuse_operations spam_mail_oper = {
	.init       = spam_mail_init,
	.getattr	= spam_mail_getattr,
	.readdir	= spam_mail_readdir,
	.create 	= spam_mail_create,
	.open		= spam_mail_open,
	.read		= spam_mail_read,
	.write		= spam_mail_write,
};

int main(int argc, char *argv[]) {
	int ret;
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
	struct fuse_cmdline_opts opts;
	struct stat stbuf;

	// This allows us to run fuse in the foreground and see printf outputs
	if (fuse_parse_cmdline(&args, &opts) != 0) {
		return 1;
	}
	fuse_opt_free_args(&args);

	if (!opts.mountpoint) {
		fprintf(stderr, "missing mountpoint parameter\n");
		return 1;
	}

	if (stat(opts.mountpoint, &stbuf) == -1) {
		fprintf(stderr ,"failed to access mountpoint %s: %s\n", 
		 		opts.mountpoint, strerror(errno));
		free(opts.mountpoint);
		return 1;
	}
	free(opts.mountpoint);

	// Make sure that the mountpoint is a directory
	if (!S_ISDIR(stbuf.st_mode)) {
		fprintf(stderr, "mountpoint is not a regular file\n");
		return 1;
	}

	// Add a fake file inside in our fake filesystem
	fake_filesystem.file_names[0] = strdup("/README.txt");
	fake_filesystem.file_contents[0] = strdup("Writing to any file in this mounted directory will send spam mail!\n");
	fake_filesystem.no_files = 1;

	ret = fuse_main(argc, argv, &spam_mail_oper, NULL);

	return ret;
}
