#include "../uwufs/uwufs.h"
#include "../uwufs/low_level_operations.h"

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/fs.h>

int main(int argc, char* argv[]) {
    // Copied from test.c
	if (argc < 2) {
		printf("Usage: %s [block device]\n", argv[0]);
		return 1;
	}

	int fd = open(argv[1], O_RDWR);
	if (fd < 0) {
		perror("Failed to access block device");
		return 1;
	}

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
	// ----

    uwufs_blk_t first_free_inode_num = -1;
    first_free_inode_num = get_first_free_inode(fd, 
                                                super_blk.ilist_start,
                                                super_blk.ilist_total_size);

    printf("First free inode: %lu\n", first_free_inode_num);

    close(fd);
}