#include "../uwufs/low_level_operations.h"
#include "../uwufs/uwufs.h"

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#include "../uwufs/mkfs_uwu.c"

int main() {
    // test append_dblk & get_dblk
    int fd = open("/dev/vdb", O_RDWR);
    if (fd < 0) {
        printf("open failed\n");
        return 1;
    }
    init_uwufs(fd, 100000, UWUFS_RESERVED_SPACE, UWUFS_ILIST_DEFAULT_PERCENTAGE);
    struct uwufs_inode inode;
    for (uwufs_blk_t i = 0; i < 300000; ++i) {
        uwufs_blk_t blk_no = i + 1;
        append_dblk(&inode, fd, i, blk_no);
        // printf("i = %lu\n", i);
        if (get_dblk(&inode, fd, i) != blk_no) {
            printf("append_dblk or get_dblk failed\n");
            return 1;
        }
    }
    // auto itr = create_dblk_itr(&inode, fd, 262656);
    // for (int i = 0; i < 100; ++i) {
    //     printf("%lu\n", dblk_itr_next(itr));
    // }

    // test remove_dblk
    // for (uwufs_blk_t i = 299999; i > 199999; --i) {
    //     if (get_dblk(&inode, fd, i) != remove_dblk(&inode, fd, i)) {
    //         printf("remove_dblk failed\n");
    //         return 1;
    //     }
    // }
    // test remove_dblks
    // remove_dblks(&inode, fd, 200000, 300000);
}
