/**
 * 	Only for testing
 *
 * 	Authors: Jason, Kay
 */

#include "../uwufs/uwufs.h"
#include "../uwufs/file_operations.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/fs.h>

#include "../uwufs/cpp/c_api.h"

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


	// ----- Test write_file -----
    int ret = 0;
	const char* test_data = "This is some test data for the file.";
	size_t data_size = strlen(test_data);

    off_t offset = 0;
	struct uwufs_inode test_inode = {0};
    uwufs_blk_t inode_num = 1111; 
	test_inode.file_size = 0;
    test_inode.file_mode = 0;
    test_inode.file_uid = 1000;
    test_inode.file_gid = 1000;
    test_inode.direct_blks[0] = 0;
    
    ssize_t status;
    int indirect_addresses = UWUFS_BLOCK_SIZE / sizeof(uwufs_blk_t);
    printf("Attempting to write to all direct blks + %d single indirect blks\n", indirect_addresses);

    //write to all direct + single indirects
    for (int i = UWUFS_DIRECT_BLOCKS; i < UWUFS_DIRECT_BLOCKS + indirect_addresses; i++) {
        printf("Writing data block %d\n", i+1);
        offset = i * UWUFS_BLOCK_SIZE;
        status = write_file(fd, test_data, data_size, offset, &test_inode, inode_num);
        if(status != data_size){
            printf("Unable to write full %ld bytes\n", data_size);
            return -1;
        }
    }


	struct uwufs_regular_file_data_blk data_blk;

    char data_in_block[UWUFS_BLOCK_SIZE];
    status = read_file(fd, (char*)data_in_block, data_size, offset, &test_inode);

    for (size_t i = 0; i < data_size; ++i) {
        if(data_in_block[i] != test_data[i]){
            printf("error 3");
            ret = -1;
            return ret;
        }
    }

    printf("New inode file size: %ld\n", test_inode.file_size);
    printf("Write file test passed!\n");

	close(fd);
	return ret;
}
