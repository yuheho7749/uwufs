/**
 * 	Only for testing
 *
 * 	Authors: Kay
 */

#include "../uwufs/uwufs.h"
#include "../uwufs/file_operations.h"

#include <stdio.h>
#include <errno.h>
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

    int i;
    //write to all direct + single indirects
    for (i = UWUFS_DIRECT_BLOCKS; i < UWUFS_DIRECT_BLOCKS + indirect_addresses; i++) {
        printf("Writing data block %d\n", i+1);
        offset = i * UWUFS_BLOCK_SIZE;
        status = write_file(fd, test_data, data_size, offset, &test_inode, inode_num);
        if(status != data_size){
            printf("Unable to write full %ld bytes\n", data_size);
            return -1;
        }
    }

    int dbl_indirect_addresses = (UWUFS_BLOCK_SIZE / sizeof(uwufs_blk_t)) *
                                 (UWUFS_BLOCK_SIZE / sizeof(uwufs_blk_t)); 

    printf("Attempting to write to all %d double indirect blks\n", dbl_indirect_addresses);

    int first_triple_indirect_addr = UWUFS_DIRECT_BLOCKS + indirect_addresses + dbl_indirect_addresses;
    // write to some double indirects
    /*
    for (i = UWUFS_DIRECT_BLOCKS + indirect_addresses; 
         i < first_triple_indirect_addr; i++) {
        if (i % indirect_addresses == 0) {
            printf("Writing data block [%d/%d]\n", i+1, first_triple_indirect_addr);
        }
        
        offset = i * UWUFS_BLOCK_SIZE;
        status = write_file(fd, test_data, data_size, offset, &test_inode, inode_num);
        if(status != data_size){
            printf("Unable to write full %ld bytes\n", data_size);
            return -1;
        }
    }*/

	struct uwufs_regular_file_data_blk data_blk;
    
    status = read_file(fd, data_blk.data, data_size, 0, &test_inode);
    if (status <= 0) {
        printf("Read 0 bytes/unsucesful\n");
        return -EIO;
    }
    printf("Read %ld bytes\n", status);

    for (size_t i = 0; i < data_size; ++i) {
        if(data_blk.data[i] != test_data[i]){
            printf("error 3\n");
            ret = -1;
            return ret;
        }
        else {
            printf("matching so far\n");
        }
    }

    printf("New inode file size: %ld\n", test_inode.file_size);
    printf("Write file test passed!\n");

	close(fd);
	return ret;
}
