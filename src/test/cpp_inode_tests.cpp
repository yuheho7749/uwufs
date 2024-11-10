#include "../uwufs/cpp/INode.h"
#include "../uwufs/low_level_operations.h"
#include "../uwufs/uwufs.h"

#include <cassert>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>


void test_direct_full(int device_fd) {
    uwufs_inode inode;
    uwufs_blk_t directs[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    memcpy(inode.direct_blks, directs, sizeof(directs));
    inode.single_indirect_blks = 0;
    inode.double_indirect_blks = 0;
    inode.triple_indirect_blks = 0;
    INode wrapper(&inode, device_fd);
    for (int i = 0; i < 10; i++) {
        auto blk = wrapper.get_dblk(i);
        assert(blk == directs[i]);
    }
}


int main() {
    constexpr char device_path[] = "/dev/vdb";
    auto device_fd = open(device_path, O_RDWR);
    assert(device_fd >= 0);
    test_direct_full(device_fd);

    close(device_fd);
    return 0;
}
