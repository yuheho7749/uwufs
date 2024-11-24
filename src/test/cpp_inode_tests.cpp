#include "../uwufs/cpp/INode.h"
#include "../uwufs/cpp/DataBlockIterator.h"
#include "../uwufs/low_level_operations.h"
#include "../uwufs/uwufs.h"

#include <cassert>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>


void test_direct_full(int device_fd) {
    uwufs_inode inode;
    uwufs_blk_t directs[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
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

void test_direct_partial(int device_fd) {
    uwufs_inode inode;
    uwufs_blk_t directs[10] = {1, 2, 3, 4, 5, 0, 0, 0, 0, 0};
    memcpy(inode.direct_blks, directs, sizeof(directs));
    inode.single_indirect_blks = 0;
    inode.double_indirect_blks = 0;
    inode.triple_indirect_blks = 0;
    INode wrapper(&inode, device_fd);
    auto itr = wrapper.dblk_itr();
    for (int i = 0; i < 5; i++) {
        auto blk = itr.next();
        assert(blk == directs[i]);
    }
    assert(itr.next() == 0);
}

void test_direct_empty(int device_fd) {
    uwufs_inode inode;
    memset(&inode, 0, sizeof(inode));
    INode wrapper(&inode, device_fd);
    auto itr = wrapper.dblk_itr();
    assert(itr.next() == 0);
}

void test_single_indirect_full(int device_fd) {
    uwufs_inode inode;
    uwufs_blk_t directs[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    memcpy(inode.direct_blks, directs, sizeof(directs));
    inode.single_indirect_blks = 11;
    inode.double_indirect_blks = 0;
    inode.triple_indirect_blks = 0;
    INode::IndirectBlock iblk;
    for (int i = 0; i < UWUFS_BLOCK_SIZE / sizeof(uwufs_blk_t); i++) {
        iblk.block_nos[i] = 20 + i;
    }
    write_blk(device_fd, &iblk, 11);
    INode wrapper(&inode, device_fd);
    auto itr = wrapper.dblk_itr();
    for (int i = 0; i < 10; i++) {
        auto blk = itr.next();
        // printf("blk: %lu\n", blk);
        assert(blk == directs[i]);
    }
    for (int i = 0; i < UWUFS_BLOCK_SIZE / sizeof(uwufs_blk_t); i++) {
        auto blk = itr.next();
        // printf("blk: %lu\n", blk);
        assert(blk == 20 + i);
    }
    assert(itr.next() == 0);
}

void test_double_indirect_full(int device_fd) {
    uwufs_inode inode;
    uwufs_blk_t directs[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    memcpy(inode.direct_blks, directs, sizeof(directs));
    inode.single_indirect_blks = 11;
    inode.double_indirect_blks = 21;
    inode.triple_indirect_blks = 0;
    INode::IndirectBlock iblk;
    for (int i = 0; i < UWUFS_BLOCK_SIZE / sizeof(uwufs_blk_t); i++) {
        iblk.block_nos[i] = 30 + i;
    }
    write_blk(device_fd, &iblk, 11);
    for (int i = 0; i < UWUFS_BLOCK_SIZE / sizeof(uwufs_blk_t); i++) {
        iblk.block_nos[i] = 1000 + i;
    }
    write_blk(device_fd, &iblk, 21);
    int cur = 2000;
    for (int i = 0; i < UWUFS_BLOCK_SIZE / sizeof(uwufs_blk_t); i++) {
        for (int j = 0; j < UWUFS_BLOCK_SIZE / sizeof(uwufs_blk_t); j++) {
            iblk.block_nos[j] = cur++;
        }
        write_blk(device_fd, &iblk, 1000 + i);
    }
    INode wrapper(&inode, device_fd);
    auto itr = wrapper.dblk_itr();
    for (int i = 0; i < 10; i++) {
        auto blk = itr.next();
        // printf("blk: %lu\n", blk);
        assert(blk == directs[i]);
    }
    for (int i = 0; i < UWUFS_BLOCK_SIZE / sizeof(uwufs_blk_t); i++) {
        auto blk = itr.next();
        // printf("blk: %lu\n", blk);
        assert(blk == 30 + i);
    }
    for (
        int i = 0;
        i < UWUFS_BLOCK_SIZE * UWUFS_BLOCK_SIZE / sizeof(uwufs_blk_t) / sizeof(uwufs_blk_t);
        i++
    ) {
        auto blk = itr.next();
        assert(blk == 2000 + i);
    }
    assert(itr.next() == 0);
}


int main() {
    constexpr char device_path[] = "/dev/vdb";
    auto device_fd = open(device_path, O_RDWR);
    assert(device_fd >= 0);
    test_direct_full(device_fd);
    test_direct_partial(device_fd);
    test_direct_empty(device_fd);
    test_single_indirect_full(device_fd);
    test_double_indirect_full(device_fd);
    close(device_fd);
    return 0;
}
