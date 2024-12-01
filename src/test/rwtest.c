/**
 * 	Only for testing
 *
 * 	Author: Jason
 */

#include "../uwufs/uwufs.h"
#include "../uwufs/file_operations.h"

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/fs.h>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s [block device]\n", argv[0]);
        return 1;
	}

	int fd = open(argv[1], O_RDWR);
	if (fd < 0) {
		perror("Failed to access block device");
		return 1;
	}

    //ssize_t bytes_written = write_file(fd, data, data_size, offset, &inode, inode_num);

	// ----- Test write_file -----
    int ret = 0;
	const char* test_data = "This is some test data for the file.";
	size_t data_size = sizeof(test_data);

    off_t offset = 0;
	struct uwufs_inode test_inode = {0};
    uwufs_blk_t inode_num = 1234; 
	test_inode.file_size = 0;
    test_inode.file_mode = 0;
    test_inode.file_uid = 1000;
    test_inode.file_gid = 1000;
    test_inode.direct_blks[0] = 0;
    

	ssize_t write_status = write_file(fd, test_data, data_size, offset, &test_inode, inode_num);
    if(write_status != data_size){
        printf("error 1");
        ret = -1;
        return ret;
    }

	struct uwufs_regular_file_data_blk data_blk;

	char* data_in_block = (char*)&data_blk;
    for (size_t i = 0; i < data_size; ++i) {
        if(data_in_block[i] != test_data[i]){
            printf("error 3");
            ret = -1;
            return ret;
        }
    }
    printf("Write file test passed!\n");

	close(fd);
	return ret;
}
