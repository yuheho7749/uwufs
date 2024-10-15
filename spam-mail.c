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


static struct fake_file {
	const char *filename;
	const char *contents;
} fake_file;


static void* spam_mail_init(struct fuse_conn_info *conn, struct fuse_config *cfg) {
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
static int spam_mail_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
	(void) fi;	

	memset(stbuf, 0, sizeof(struct stat));
	if (strcmp(path, "/") == 0) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2; 	// A directory has at least 2 hard links because "." 
								// inside itself also refers to itself
	} else if (strcmp(path + 1, fake_file.filename) == 0) { 
		// path + 1 because the filepath is "/filename.txt" and we don't want the "/"
		stbuf->st_mode = S_IFREG | 0644;
		stbuf->st_nlink = 1;

		stbuf->st_size = strlen(fake_file.contents);
		// assuming block size of 4k bytes (4096 bytes)
		stbuf->st_blocks = (stbuf->st_size + 4095) / 4096;

		stbuf->st_uid = getuid();
		stbuf->st_gid = getgid();

		stbuf->st_atime = time(NULL);
		stbuf->st_mtime = stbuf->st_atime;
		stbuf->st_ctime = stbuf->st_atime;
	} else {
		// TODO: This is all hardcoded, might want to actually allow
		// user to make their own files
		return -ENOENT;
	}

	return 0;
}

/**
 *	Similar to getattr, but for directories
 */
static int spam_mail_readdir(const char *path, void *buf, fuse_fill_dir_t filler, 
							 off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags) {
	(void) offset;
	(void) fi;
	(void) flags;

	if (strcmp(path, "/") != 0) {
		return -ENOENT;
	}
	
	filler(buf, ".", NULL, 0, FUSE_FILL_DIR_DEFAULTS);
	filler(buf, "..", NULL, 0, FUSE_FILL_DIR_DEFAULTS);
	filler(buf, fake_file.filename, NULL, 0, FUSE_FILL_DIR_DEFAULTS);

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
static int spam_mail_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
	// TODO:	


	return 0;
}

static int spam_mail_open(const char *path, struct fuse_file_info *fi) {

	// TEMP: Only care about our fake file
	if (strcmp(path + 1, fake_file.filename) != 0) {
		return -ENOENT;
	}

	// TODO: Check the file permissions vs the open file permission in fi
	// if ((fi->flags & O_ACCMODE) != O_RDONLY)
	// 	return -EACCES;

	return 0;
}

static int spam_mail_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
	(void) fi;

	// TEMP: Only care about our fake file
	if(strcmp(path + 1, fake_file.filename) != 0) {
		return -ENOENT;
	}

	int len;
	len = strlen(fake_file.contents);
	if (offset < len) {
		if (offset + size > len) {
			size = len - offset;
		}
		memcpy(buf, fake_file.contents + offset, size);
	} else {
		size = 0;
	}

	return size;
}

static int spam_mail_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
	
	(void) path;
	(void) fi;

	// TEMP: Only care about our fake file
	if(strcmp(path + 1, fake_file.filename) != 0) {
		return -ENOENT;
	}

	// ISSUE: SECURITY ISSUE!!!
	// Taking in raw user input as command is bad!!!
	char command[10000];

	// TODO: Change to professor's email: rich@cs.ucsb.edu
	snprintf(command, 10000, "echo \"%s\" | mail -s \"Fuse Mail!\" joseph_ng@ucsb.edu", buf);

	printf("Attempting to send mail\n");
	system(command);
	printf("Sent mail\n");

	return size;
}

static const struct fuse_operations spam_mail_oper = {
	.init       = spam_mail_init, 		// technically not needed, but I'm enabling direct_io to avoid page caching
	.getattr	= spam_mail_getattr,
	.readdir	= spam_mail_readdir,
	.create 	= spam_mail_create,		// TODO:
	.open		= spam_mail_open,
	.read		= spam_mail_read, 		// technically not needed for phase 1
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
		fprintf(stderr ,"failed to access mountpoint %s: %s\n", opts.mountpoint, strerror(errno));
		free(opts.mountpoint);
		return 1;
	}
	free(opts.mountpoint);

	// Make sure that the mountpoint is a directory
	if (!S_ISDIR(stbuf.st_mode)) {
		fprintf(stderr, "mountpoint is not a regular file\n");
		return 1;
	}

	// Add a fake file inside the mounted fs to make sure the mounting is correct
	fake_file.filename = strdup("README.txt");
	fake_file.contents = strdup("Writing to any file in this mounted directory will send spam mail!\n");

	ret = fuse_main(argc, argv, &spam_mail_oper, NULL);

	return 0;
}
