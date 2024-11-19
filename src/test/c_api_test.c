#include "../uwufs/low_level_operations.h"
#include "../uwufs/uwufs.h"

#include <string.h>
#include <stdio.h>

int main() {
    struct uwufs_inode inode;
    uwufs_blk_t directs[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    memcpy(inode.direct_blks, directs, sizeof(directs));
    uwufs_blk_t i = get_dblk(&inode, 0, 0);
    printf("blk: %lu\n", i);
    return 0;
}