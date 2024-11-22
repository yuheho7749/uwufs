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
    for (uwufs_blk_t i = 0; i < 10000; ++i) {
        uwufs_blk_t blk_no = i + 1;
        append_dblk(&inode, fd, i, blk_no);
        printf("blk_no: %lu\n", blk_no);
        if (get_dblk(&inode, fd, i) != blk_no) {
            printf("append_dblk or get_dblk failed\n");
            return 1;
        }
    }

    // test remove_dblk
    for (uwufs_blk_t i = 999; i > 0; --i) {
        if (get_dblk(&inode, fd, i) != remove_dblk(&inode, fd, i)) {
            printf("remove_dblk failed\n");
            return 1;
        }
    }
}