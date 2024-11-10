#include "DataBlockIterator.h"
#include "../low_level_operations.h"

#ifndef assert_low_level_operations
#define assert_low_level_operations(ret) \
    if (ret < 0) { \
        perror("Low level operation failed"); \
        exit(1); \
    }
#endif


IndirectBlockIterator::IndirectBlockIterator(uwufs_blk_t iblk_no, int device_fd, const uwufs_inode* inode, uwufs_blk_t start_index)
: device_fd(device_fd), inode(inode), current_index(start_index) {
    if (iblk_no != 0) {
        iblk = std::make_unique<INode::IndirectBlock>();
        auto ret = read_blk(device_fd, iblk.get(), iblk_no);
#ifdef DEBUG
        assert_low_level_operations(ret);
#endif
    }
}

SingleIndirectBlockIterator::value_type SingleIndirectBlockIterator::next() {
    if (!iblk || current_index >= UWUFS_BLOCK_SIZE / sizeof(uwufs_blk_t)) {
        return 0;
    }
    return iblk->block_nos[current_index++];
}

std::unique_ptr<IndirectBlockIterator> SingleIndirectBlockIterator::next_itr() {
    return std::make_unique<DoubleIndirectBlockIterator>(inode->double_indirect_blks, device_fd, inode, 0, 0);
}

DoubleIndirectBlockIterator::DoubleIndirectBlockIterator(uwufs_blk_t iblk_no, int device_fd, const uwufs_inode* inode, uwufs_blk_t start_i, uwufs_blk_t start_j)
: IndirectBlockIterator(iblk_no, device_fd, inode, start_i) {
    if (iblk) { // this double indirect block is not empty
        single_itr = std::make_unique<SingleIndirectBlockIterator>(iblk->block_nos[start_i], device_fd, inode, start_j);
    }
}

DoubleIndirectBlockIterator::value_type DoubleIndirectBlockIterator::next() {
    if (!single_itr) {
        return 0;
    }
    auto blk_no = single_itr->next();
    if (blk_no == 0) {  // change to the next single indirect block
        single_itr = std::make_unique<SingleIndirectBlockIterator>(iblk->block_nos[++current_index], device_fd, inode, 0);
        blk_no = single_itr->next();
    }
    return blk_no;
}

std::unique_ptr<IndirectBlockIterator> DoubleIndirectBlockIterator::next_itr() {
    return std::make_unique<TripleIndirectBlockIterator>(inode->triple_indirect_blks, device_fd, inode, 0, 0, 0);
}

TripleIndirectBlockIterator::TripleIndirectBlockIterator(uwufs_blk_t iblk_no, int device_fd, const uwufs_inode* inode, uwufs_blk_t start_i, uwufs_blk_t start_j, uwufs_blk_t start_k)
: IndirectBlockIterator(iblk_no, device_fd, inode, start_i) {
    if (iblk) { // this triple indirect block is not empty
        double_itr = std::make_unique<DoubleIndirectBlockIterator>(iblk->block_nos[start_i], device_fd, inode, start_j, start_k);
    }
}

TripleIndirectBlockIterator::value_type TripleIndirectBlockIterator::next() {
    if (!double_itr) {
        return 0;
    }
    auto blk_no = double_itr->next();
    if (blk_no == 0) {  // change to the next double indirect block
        double_itr = std::make_unique<DoubleIndirectBlockIterator>(iblk->block_nos[++current_index], device_fd, inode, 0, 0);
        blk_no = double_itr->next();
    }
    return blk_no;
}

std::unique_ptr<IndirectBlockIterator> TripleIndirectBlockIterator::next_itr() {
    return nullptr; // no more indirect blocks after triple indirect block
}

DataBlockIterator::DataBlockIterator(const uwufs_inode* inode, int device_fd, uwufs_blk_t start_index)
: inode(inode), device_fd(device_fd) {
    if (start_index < INode::LEVEL_0_BLOCKS) {  // direct blocks
        current_index = start_index;
    }
    else if (start_index < INode::LEVEL_1_BLOCKS) {
        itr = std::make_unique<SingleIndirectBlockIterator>(inode->single_indirect_blks, device_fd, inode, start_index - INode::LEVEL_0_BLOCKS);
    }
    else if (start_index < INode::LEVEL_2_BLOCKS) {
        start_index -= INode::LEVEL_1_BLOCKS;
        auto i = start_index / (INode::LEVEL_2_BLOCKS - INode::LEVEL_1_BLOCKS);
        auto j = start_index % (INode::LEVEL_2_BLOCKS - INode::LEVEL_1_BLOCKS);
        itr = std::make_unique<DoubleIndirectBlockIterator>(inode->double_indirect_blks, device_fd, inode, i, j);
    }
    else {
        start_index -= INode::LEVEL_2_BLOCKS;
        auto i = start_index / (INode::LEVEL_3_BLOCKS - INode::LEVEL_2_BLOCKS);
        auto rem = start_index % (INode::LEVEL_3_BLOCKS - INode::LEVEL_2_BLOCKS);
        auto j = rem / (INode::LEVEL_2_BLOCKS - INode::LEVEL_1_BLOCKS);
        auto k = rem % (INode::LEVEL_2_BLOCKS - INode::LEVEL_1_BLOCKS);
        itr = std::make_unique<TripleIndirectBlockIterator>(inode->triple_indirect_blks, device_fd, inode, i, j, k);
    }
}

DataBlockIterator::value_type DataBlockIterator::next() {
    if (!itr) { // in direct blocks
        if (current_index >= INode::LEVEL_0_BLOCKS) {
            itr = std::make_unique<SingleIndirectBlockIterator>(inode->single_indirect_blks, device_fd, inode, 0);
        }
        else {
            return inode->direct_blks[current_index++];
        }
    }
    auto blk_no = itr->next();
    if (blk_no == 0) {  // change to the next indirect block
        itr = itr->next_itr();
        if (!itr) {
            return 0;
        }
        blk_no = itr->next();
    }
    return blk_no;
}