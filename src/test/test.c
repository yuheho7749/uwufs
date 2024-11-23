/**
 * 	Only for testing
 *
 * 	Author: Joseph
 */

#include "../uwufs/uwufs.h"
#include "../uwufs/low_level_operations.h"

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/fs.h>


int main(int argc, char* argv[]) {
	// ----- Print basic struct sizes -----
	printf("Size of uwufs_super_blk is %ld\n", sizeof(struct uwufs_super_blk));
	printf("Size of uwufs_inode is %ld\n", sizeof(struct uwufs_inode));
	printf("Size of uwufs_inode_blk is %ld\n", sizeof(struct uwufs_inode_blk));
	printf("Size of uwufs_directory_file_entry is %ld\n",
		   sizeof(struct uwufs_directory_file_entry));
	printf("Size of uwufs_directory_blk is %ld\n", 
		   sizeof(struct uwufs_directory_data_blk));
	printf("Size of uwufs_regular_file_blk is %ld\n",
		   sizeof(struct uwufs_regular_file_data_blk));
	printf("\n");


	// ----- Test reading from actual blk device -----
	if (argc < 2) {
		printf("Usage: %s [block device]\n", argv[0]);
		return 1;
	}

	int fd = open(argv[1], O_RDWR);
	if (fd < 0) {
		perror("Failed to access block device");
		return 1;
	}
	
	int ret = 0;

	uwufs_blk_t blk_dev_size;
	ret = ioctl(fd, BLKGETSIZE64, &blk_dev_size);
	if (ret < 0) {
		perror("Not a block device or cannot determine size of device");
		close(fd);
		return 1;
	}


	// ----- Read super block info -----
	struct uwufs_super_blk super_blk;
	ssize_t status = read_blk(fd, &super_blk, 0);
	if (status < 0) {
		perror("Failed to read super blk");
		close(fd);
		exit(1);
	}
	printf("Super Block:\n");
	printf("\ttotal_blks: %lu\n", super_blk.total_blks);
	printf("\tilist blk range: [%lu, %lu)\n", super_blk.ilist_start,
		super_blk.ilist_start + super_blk.ilist_total_size);
	printf("\tfreelist blk range: [%lu, %lu)\n", super_blk.freelist_start,
		super_blk.freelist_start + super_blk.freelist_total_size);
	printf("\tfreelist_head: %lu\n", super_blk.freelist_head);

	// ----- Read root directory inode info -----
	struct uwufs_inode root_directory_inode;
	status = read_inode(fd, &root_directory_inode, UWUFS_ROOT_DIR_INODE);
	if (status < 0) {
		perror("Failed to read root dir inode");
		close(fd);
		exit(1);
	}
	printf("Root Directory Inode:\n");
	uint16_t f_mode = root_directory_inode.file_mode;
	uint64_t dir_size = root_directory_inode.file_size;
	printf("\tFile type: %d\n", (f_mode & F_TYPE_BITS) >> 12);
	printf("\tFile perm: %o\n", f_mode & F_PERM_BITS);
	printf("\tDir size (raw): %lu\n", dir_size);
	printf("\tDir links: %d\n", root_directory_inode.file_links_count);
	printf("\tDir uid: %d\n", root_directory_inode.file_uid);
	printf("\tDir gid: %d\n", root_directory_inode.file_gid);

	// ----- Read freelist head using super blk info -----
	struct uwufs_directory_data_blk dir_data_blk;
	status = read_blk(fd, &dir_data_blk, root_directory_inode.direct_blks[0]);
	if (status < 0) {
		perror("Failed to read first data blk of root dir");
		close(fd);
		exit(1);
	}

	// ---- Check root dir blk numbers to make sure unused set to 0 ----
	printf("Root Directory Direct Block Numbers (all blocks)\n");
	uwufs_blk_t i;
	for (i = 0; i < UWUFS_DIRECT_BLOCKS; i++) {
		printf("Direct block %lu: %lu\n", i, root_directory_inode.direct_blks[i]);
	}

	printf("Root Directory Entries (first block only):\n");
	for (i = 0; i < UWUFS_BLOCK_SIZE/sizeof(struct uwufs_directory_file_entry); i ++) {
		if (dir_data_blk.file_entries[i].inode_num <= 0) {
			continue;
		}
		printf("\t%lu\t%s\n", dir_data_blk.file_entries[i].inode_num,
		 	   dir_data_blk.file_entries[i].file_name);
	}
	status = read_blk(fd, &dir_data_blk, root_directory_inode.direct_blks[1]);
	if (status < 0) {
		perror("Failed to read first data blk of root dir");
		close(fd);
		exit(1);
	}
	printf("Root Directory Direct Block Numbers (all blocks)\n");
	for (i = 0; i < UWUFS_DIRECT_BLOCKS; i++) {
		printf("Direct block %lu: %lu\n", i, root_directory_inode.direct_blks[i]);
	}

	printf("Root Directory Entries (second block):\n");
	for (i = 0; i < UWUFS_BLOCK_SIZE/sizeof(struct uwufs_directory_file_entry); i ++) {
		if (dir_data_blk.file_entries[i].inode_num <= 0) {
			continue;
		}
		printf("\t%lu\t%s\n", dir_data_blk.file_entries[i].inode_num,
		 	   dir_data_blk.file_entries[i].file_name);
	}

	// ----- Read freelist head using super blk info -----
	struct uwufs_free_data_blk free_blk;
	status = read_blk(fd, &free_blk, super_blk.freelist_head);
	if (status < 0) {
		perror("Failed to read freelist head");
		close(fd);
		exit(1);
	}
	printf("Freelist head\n");
	printf("\tNext free block num: %lu\n", free_blk.next_free_blk);

	// ----- Read last freelist block -----
	struct uwufs_free_data_blk last_free_blk;
	status = read_blk(fd, &last_free_blk, super_blk.freelist_start +
				   	  super_blk.freelist_total_size - 1);
	if (status < 0) {
		perror("Failed to read last freelist block");
		close(fd);
		exit(1);
	}
	printf("Last freelist block: %lu\n", super_blk.freelist_start +
		super_blk.freelist_total_size - 1);
	printf("\tNext free block num: %lu\n", last_free_blk.next_free_blk);

	// ----- Read random inode info -----
	struct uwufs_inode random_inode;
	status = read_inode(fd, &random_inode, 23197);
	if (status < 0) {
		perror("Failed to read random inode");
		close(fd);
		exit(1);
	}
	printf("Random Inode:\n");
	f_mode = random_inode.file_mode;
	printf("\tFile type: %d\n", (f_mode & F_TYPE_BITS) >> 12);
	printf("\tFile perm: %o\n", f_mode & F_PERM_BITS);
	printf("\tDir size (raw): %lu\n", random_inode.file_size);

	close(fd);
	return ret;
}
